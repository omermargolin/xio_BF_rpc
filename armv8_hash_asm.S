
    .text

// uint32_t FastSha1Data(uint8_t *state_ptr,
//                       uint8_t *data_ptr,
//                       uint32_t data_len,
//                       uint8_t *unused);

// Global Register assignments.  These registers have only one use.
#define RET_VALUE   X0
#define STATE_PTR   X0
#define DATA_PTR    X1
#define DATA_LEN    X2

#define X_BYTE_CNT   X3
#define SHA1TBL_PTR  X4

#define Q_STATE3_0   Q0
#define V_STATE3_0   V0
#define S_STATE4     S1
#define V_STATE4     V1
#define Q_DCBA       Q2
#define S_DCBA       S2
#define V_DCBA       V2
#define V_E          V3
#define E_EVEN       S3       // V3.S[0]
#define E_ODD        S4       // V4.S[0]
#define W_PLUS_K     V5
#define W3W2W1W0     V6.4S
#define W7W6W5W4     V7.4S
#define W11W10W9W8   V29.4S
#define W15W14W13W12 V30.4S
#define KxKxKxKx     V31.4S

.macro LD_BIG_ENDIAN_32B  Vreg1, Vreg2, addrReg, tmp1, tmp2, tmp3, tmp4
    ldr \tmp1, [\addrReg], 8
    ldr \tmp2, [\addrReg], 8
    ldr \tmp3, [\addrReg], 8
    ldr \tmp4, [\addrReg], 8
    rev \tmp1, \tmp1
    rev \tmp2, \tmp2
    rev \tmp3, \tmp3
    rev \tmp4, \tmp4
    ins \Vreg1\().D[0], \tmp1
    ins \Vreg1\().D[1], \tmp2
    ins \Vreg2\().D[0], \tmp3
    ins \Vreg2\().D[1], \tmp4
    rev64 \Vreg1\().4S, \Vreg1\().4S
    rev64 \Vreg2\().4S, \Vreg2\().4S
.endm

.macro SHA1C_4RNDS  DCBA, A, E, NEXT_E, K, W3_0, W7_4, W11_8, W15_12
    sha1h   \NEXT_E,     \A
    add     W_PLUS_K.4S, \W3_0\().4S,  \K\().4S
    sha1su0 \W3_0\().4S, \W7_4\().4S,  \W11_8\().4S
    sha1c   \DCBA,       \E,           W_PLUS_K.4S
    sha1su1 \W3_0\().4S, \W15_12\().4S
.endm

.macro SHA1P_4RNDS  DCBA, A, E, NEXT_E, K, W3_0, W7_4, W11_8, W15_12
    sha1h   \NEXT_E,     \A
    add     W_PLUS_K.4S, \W3_0\().4S,  \K\().4S
    sha1su0 \W3_0\().4S, \W7_4\().4S,  \W11_8\().4S
    sha1p   \DCBA,       \E,           W_PLUS_K.4S
    sha1su1 \W3_0\().4S, \W15_12\().4S
.endm

.macro SHA1P_B_4RNDS  DCBA, A, E, NEXT_E, K, W3_0
    sha1h   \NEXT_E,     \A
    add     W_PLUS_K.4S, \W3_0\().4S,  \K\().4S
    sha1p   \DCBA,       \E,           W_PLUS_K.4S
.endm

.macro SHA1M_4RNDS  DCBA, A, E, NEXT_E, K, W3_0, W7_4, W11_8, W15_12
    sha1h   \NEXT_E,     \A
    add     W_PLUS_K.4S, \W3_0\().4S,  \K\().4S
    sha1su0 \W3_0\().4S, \W7_4\().4S,  \W11_8\().4S
    sha1m   \DCBA,       \E,           W_PLUS_K.4S
    sha1su1 \W3_0\().4S, \W15_12\().4S
.endm



    .align  4
    .type   FastSha1Data, @function
    .global FastSha1Data
FastSha1Data:
    // Prologue - currently no registers need saving/restoring.
    eor  V_STATE4.16B, V_STATE4.16B, V_STATE4.16B  // Clear V_STATE4
    cmp  DATA_LEN,     #64
    mov  X_BYTE_CNT,   #0

    // First load the state variables.
    ldr  Q_STATE3_0,  [STATE_PTR]      // Load STATE3_0
    adrp SHA1TBL_PTR, SHA1_K           // load address of SHA1_K table
    ldr  S_STATE4,    [STATE_PTR, #16] // Load STATE4
    add  SHA1TBL_PTR, SHA1TBL_PTR, #:lo12:SHA1_K
    b.lt Sha1Return   // Return if data_len < 64

Sha1BlkLoop:
    // while (64 <= data_len)
    //     for (t = 0;  t <= 15;  t++)
    //         w[t] = Load4BigEndianBytes(&data_ptr[t * 4]);

    // Load buffer into W0, W1, ... W15 - I.e. Q6, Q7, Q16 and Q17.
    // Load the first SHA1_K table constant into S18.
    // ldp Q6,  Q7, [DATA_PTR], 32
    // rev64 V6.4S,  V6.4S
    // rev64 V7.4S,  V7.4S
    LD_BIG_ENDIAN_32B  V6,  V7,  DATA_PTR, X10, X11, X12, X13
    LD_BIG_ENDIAN_32B  V16, V17, DATA_PTR, X10, X11, X12, X13
    ldr S18,     [SHA1TBL_PTR]
    dup V18.4S,  V18.S[0]

    orr V20.16B, V6.16B,  V6.16B
    orr V21.16B, V7.16B,  V7.16B
    orr V22.16B, V16.16B, V16.16B
    orr V23.16B, V17.16B, V17.16B

    // DCBA = STATE3_0
    // E    = STATE4
    mov V_DCBA.16B, V_STATE3_0.16B
    mov V_E.8B,     V_STATE4.8B

    // Rounds 0-19:
    // Four rounds where Q_DCBA=d:c:b:a, S3=e, V6=w3:w2:w1:w0, V18=K0:K0:K0:K0
    // T  = 0:w15:w14:w13 ^ w11:w10:w9:w8 ^ w5:w4:w3:w2 ^ w3:w2:w1:w0;
    // V5 = w19:w18:w17:w16 =>
    //      w16 = ROL(w13 ^ w8  ^ w2 ^ w0, 1);
    //      w17 = ROL(w14 ^ w9  ^ w3 ^ w1, 1);
    //      w18 = ROL(w15 ^ w10 ^ w4 ^ w2, 1);
    //      w19 = ROL(      w11 ^ w5 ^ w3, 1) ^ ROL(w13 ^ w8  ^ w2 ^ w0, 2);
    SHA1C_4RNDS  Q_DCBA, S_DCBA, E_EVEN, E_ODD,   V18,  V6,  V7,  V16, V17
    SHA1C_4RNDS  Q_DCBA, S_DCBA, E_ODD,  E_EVEN,  V18,  V7,  V16, V17, V6
    SHA1C_4RNDS  Q_DCBA, S_DCBA, E_EVEN, E_ODD,   V18,  V16, V17, V6,  V7
    SHA1C_4RNDS  Q_DCBA, S_DCBA, E_ODD,  E_EVEN,  V18,  V17, V6,  V7,  V16
    SHA1C_4RNDS  Q_DCBA, S_DCBA, E_EVEN, E_ODD,   V18,  V6,  V7,  V16, V17

    // Rounds 20-39:
    ldr S18,    [SHA1TBL_PTR, 4]
    dup V18.4S, V18.S[0]
    SHA1P_4RNDS  Q_DCBA, S_DCBA, E_ODD,  E_EVEN,  V18,  V7,  V16, V17, V6
    SHA1P_4RNDS  Q_DCBA, S_DCBA, E_EVEN, E_ODD,   V18,  V16, V17, V6,  V7
    SHA1P_4RNDS  Q_DCBA, S_DCBA, E_ODD,  E_EVEN,  V18,  V17, V6,  V7,  V16
    SHA1P_4RNDS  Q_DCBA, S_DCBA, E_EVEN, E_ODD,   V18,  V6,  V7,  V16, V17
    SHA1P_4RNDS  Q_DCBA, S_DCBA, E_ODD,  E_EVEN,  V18,  V7,  V16, V17, V6

    // Rounds 40-59:
    ldr S18,    [SHA1TBL_PTR, 8]
    dup V18.4S, V18.S[0]
    SHA1M_4RNDS  Q_DCBA, S_DCBA, E_EVEN, E_ODD,   V18,  V16, V17, V6,  V7
    SHA1M_4RNDS  Q_DCBA, S_DCBA, E_ODD,  E_EVEN,  V18,  V17, V6,  V7,  V16
    SHA1M_4RNDS  Q_DCBA, S_DCBA, E_EVEN, E_ODD,   V18,  V6,  V7,  V16, V17
    SHA1M_4RNDS  Q_DCBA, S_DCBA, E_ODD,  E_EVEN,  V18,  V7,  V16, V17, V6
    SHA1M_4RNDS  Q_DCBA, S_DCBA, E_EVEN, E_ODD,   V18,  V16, V17, V6,  V7

    // Rounds 60-79:
    ldr S18,    [SHA1TBL_PTR, 12]
    dup V18.4S, V18.S[0]
    SHA1P_4RNDS    Q_DCBA, S_DCBA, E_ODD,  E_EVEN,  V18,  V17, V6, V7, V16
    SHA1P_B_4RNDS  Q_DCBA, S_DCBA, E_EVEN, E_ODD,   V18,  V6
    SHA1P_B_4RNDS  Q_DCBA, S_DCBA, E_ODD,  E_EVEN,  V18,  V7
    SHA1P_B_4RNDS  Q_DCBA, S_DCBA, E_EVEN, E_ODD,   V18,  V16
    SHA1P_B_4RNDS  Q_DCBA, S_DCBA, E_ODD,  E_EVEN,  V18,  V17


StepE:
    // Add this block's hash to the hash result so far.
    // STATE3_0 += DCBA
    // STATE4   += E
    add V_STATE3_0.4S, V_STATE3_0.4S, V_DCBA.4S
    add V_STATE4.4S,   V_STATE4.4S,   V_E.4S

    sub  DATA_LEN,   DATA_LEN,   #64
    add  X_BYTE_CNT, X_BYTE_CNT, #64
    cmp  DATA_LEN, #64
    b.ge Sha1BlkLoop


Sha1StoreState:
    // Store state back
    str Q_STATE3_0, [STATE_PTR]      // Store STATE3_0 back to memory.
    str S_STATE4,   [STATE_PTR, #16] // Store STATE4   back to memory.

Sha1Return:
    mov RET_VALUE, X_BYTE_CNT
    ret

    .size FastSha1Data, . - FastSha1Data


#undef RET_VALUE
#undef STATE_PTR
#undef DATA_PTR
#undef DATA_LEN

#undef X_BYTE_CNT
#undef SHA1TBL_PTR

#undef Q_STATE3_0
#undef V_STATE3_0
#undef S_STATE4
#undef V_STATE4
#undef Q_DCBA
#undef S_DCBA
#undef V_DCBA
#undef V_E
#undef E_EVEN
#undef E_ODD
#undef W_PLUS_K
#undef W3W2W1W0
#undef W7W6W5W4
#undef W11W10W9W8
#undef W15W14W13W12
#undef KxKxKxKx



// uint32_t FastSha2_256Data(uint8_t *state_ptr,
//                           uint8_t *data_ptr,
//                           uint32_t data_len,
//                           uint8_t *unused);

// Global Register assignments.  These registers have only one use.
#define RET_VALUE  X0
#define STATE_PTR  X0
#define DATA_PTR   X1
#define DATA_LEN   X2

#define X_BYTE_CNT    X3
#define CNT           X4
#define SHA256TBL_PTR X5

#define Q_STATE3_0  Q0
#define V_STATE3_0  V0
#define Q_STATE7_4  Q1
#define V_STATE7_4  V1
#define Q_DCBA      Q2
#define V_DCBA      V2
#define Q_HGFE      Q3
#define V_HGFE      V3
#define Q_TMP       Q4
#define V_TMP       V4
#define V_W_PLUS_K  V5
#define V_DATA3_0   V6
#define V_DATA7_4   V7
#define V_DATA11_8  V16
#define V_DATA15_12 V17
#define Q_K3_0      Q18
#define V_K3_0      V18
#define Q_K7_4      Q19
#define V_K7_4      V19
#define Q_K11_8     Q20
#define V_K11_8     V20
#define Q_K15_12    Q21
#define V_K15_12    V21


.macro SHA256_4RNDS  K,  W3_0, W7_4, W11_8, W15_12
    add      V_W_PLUS_K.4S, \W3_0\().4S, \K\().4S
    mov      V_TMP.16B,     V_DCBA.16B
    sha256h  Q_DCBA,        Q_HGFE,      V_W_PLUS_K.4S
    sha256h2 Q_HGFE,        Q_TMP,       V_W_PLUS_K.4S

.ifnb \W15_12
    sha256su0 \W3_0\().4S, \W7_4\().4S
    sha256su1 \W3_0\().4S, \W11_8\().4S, \W15_12\().4S
.endif
.endm


    .align  4
    .type   FastSha2_256Data, @function
    .global FastSha2_256Data
FastSha2_256Data:
    // Prologue - currently no registers need saving/restoring.
    ldp  Q_STATE3_0, Q_STATE7_4, [STATE_PTR]  // Load state.
    cmp  DATA_LEN,   #64
    mov  X_BYTE_CNT, #0
    b.lt Fast256Return  // Return if data_len < 64

Sha256BlkLoop:
    adrp SHA256TBL_PTR, SHA2_256K // load addr of SHA2_256K table
    add  SHA256TBL_PTR, SHA256TBL_PTR, #:lo12:SHA2_256K

    // while (64 <= data_len)
    //     for (t = 0;  t <= 15;  t++)
    //         w[t] = Load4BigEndianBytes(&data_ptr[t * 4]);

    // Load input data into W0, W1, ... W15 - I.e. DATA3_0, DATA7_4, DATA11_8
    // and DATA15_12.
    // Load the first 16 constants from the SHA2_256K table (pointed to
    // by SHA256TBL_PTR) into K3_0, K7_4, K11_8 and K15_12.
    LD_BIG_ENDIAN_32B V_DATA3_0,  V_DATA7_4,   DATA_PTR, X10, X11, X12, X13
    LD_BIG_ENDIAN_32B V_DATA11_8, V_DATA15_12, DATA_PTR, X10, X11, X12, X13
    ldp Q_K3_0,  Q_K7_4,   [SHA256TBL_PTR], 32
    ldp Q_K11_8, Q_K15_12, [SHA256TBL_PTR], 32

    // DCBA = STATE3_0
    // HGFE = STATE7_4
    mov  V_DCBA.16B, V_STATE3_0.16B
    mov  V_HGFE.16B, V_STATE7_4.16B

    // Need to execute the following loop three times.
    mov CNT, #3
FastSha256Loop:
    SHA256_4RNDS V_K3_0,   V_DATA3_0,  V_DATA7_4,  V_DATA11_8,  V_DATA15_12
    SHA256_4RNDS V_K7_4,   V_DATA7_4,  V_DATA11_8, V_DATA15_12, V_DATA3_0
    ldp          Q_K3_0,   Q_K7_4,     [SHA256TBL_PTR], 32

    SHA256_4RNDS V_K11_8,  V_DATA11_8,  V_DATA15_12, V_DATA3_0, V_DATA7_4
    SHA256_4RNDS V_K15_12, V_DATA15_12, V_DATA3_0,   V_DATA7_4, V_DATA11_8
    ldp          Q_K11_8,  Q_K15_12,    [SHA256TBL_PTR], 32

    sub  CNT, CNT, #1
    cbnz CNT, FastSha256Loop

Last16Rounds:
    SHA256_4RNDS V_K3_0,    V_DATA3_0
    SHA256_4RNDS V_K7_4,    V_DATA7_4
    SHA256_4RNDS V_K11_8,   V_DATA11_8
    SHA256_4RNDS V_K15_12,  V_DATA15_12

    // Add this block's hash (a, b, c, d, e, f, g, h) to the hash result so far.
    // a += state[0];
    // b += state[1];
    // c += state[2];
    // d += state[3];
    // e += state[4];
    // f += state[5];
    // g += state[6];
    // h += state[7];
    add V_STATE3_0.4S, V_STATE3_0.4S, V_DCBA.4S
    add V_STATE7_4.4S, V_STATE7_4.4S, V_HGFE.4S

    sub  DATA_LEN,   DATA_LEN,   #64
    add  X_BYTE_CNT, X_BYTE_CNT, #64
    cmp  DATA_LEN, #64
    b.ge Sha256BlkLoop


Fast256StoreState:
    // Store state variables in registers back into the STATE memory block.
    stp Q_STATE3_0, Q_STATE7_4, [STATE_PTR]   // Store state.

Fast256Return:
    mov RET_VALUE, X_BYTE_CNT
    ret

    .size FastSha2_256Data, . - FastSha2_256Data


#undef RET_VALUE
#undef STATE_PTR
#undef DATA_PTR
#undef DATA_LEN

#undef X_BYTE_CNT
#undef CNT
#undef SHA256TBL_PTR

#undef Q_STATE3_0
#undef V_STATE3_0
#undef Q_STATE7_4
#undef V_STATE7_4
#undef Q_DCBA
#undef V_DCBA
#undef Q_HGFE
#undef V_HGFE
#undef Q_TMP
#undef V_TMP
#undef V_W_PLUS_K
#undef V_DATA3_0
#undef V_DATA7_4
#undef V_DATA11_8
#undef V_DATA15_12
#undef Q_K3_0
#undef V_K3_0
#undef Q_K7_4
#undef V_K7_4
#undef Q_K11_8
#undef V_K11_8
#undef Q_K15_12
#undef V_K15_12



    .data

//  .global SHA1_K
    .align  4
    .type   SHA1_K, @object
    .size   SHA1_K, 16
SHA1_K:
    .long  0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6


//  .global SHA2_256K
    .align  4
    .type   SHA2_256K, @object
    .size   SHA2_256K, 256
SHA2_256K:
    .long  0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5
    .long  0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5
    .long  0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3
    .long  0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174
    .long  0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC
    .long  0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA
    .long  0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7
    .long  0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967
    .long  0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13
    .long  0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85
    .long  0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3
    .long  0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070
    .long  0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5
    .long  0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3
    .long  0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208
    .long  0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
