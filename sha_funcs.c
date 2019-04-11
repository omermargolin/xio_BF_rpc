#include "stdint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sha_funcs.h"

static uint8_t SHA1_INIT[] =
{
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10,
    0xF0, 0xE1, 0xD2, 0xC3
};

static uint8_t SHA2_256_INIT[] =
{
    0x67, 0xE6, 0x09, 0x6A, 0x85, 0xAE, 0x67, 0xBB,
    0x72, 0xF3, 0x6E, 0x3C, 0x3A, 0xF5, 0x4F, 0xA5,
    0x7F, 0x52, 0x0E, 0x51, 0x8C, 0x68, 0x05, 0x9B,
    0xAB, 0xD9, 0x83, 0x1F, 0x19, 0xCD, 0xE0, 0x5B
};

#define STORE_BE8(ptr, data) \
    ({ * ((uint64_t *) ptr) = __builtin_bswap64(data); })


void Sha1(uint8_t *data_ptr, uint32_t data_len, uint8_t *hash_result)
{
    uint64_t total_bit_len;
    uint32_t num_whole_blks, whole_blks_len;
    uint8_t  in_buf[64], state[20];

    total_bit_len = 8 * data_len;
    memcpy(&state[0], SHA1_INIT, sizeof(state));

    // Do an even number of 64-byte SHA1 input data blocks:
    num_whole_blks = data_len / 64;

    if (num_whole_blks != 0)
    {
        whole_blks_len = 64 * num_whole_blks;

        FastSha1Data(state, data_ptr, whole_blks_len);
        data_ptr += whole_blks_len;
        data_len -= whole_blks_len;
    }

    // Now do the final blk or 2.  Note that data_len MUST be <= 63 here.
    memset(in_buf, 0, sizeof(in_buf));
    if (data_len != 0)
        memcpy(in_buf, data_ptr, data_len);


    in_buf[data_len] = 0x80;
    if (56 <= data_len)
    {
        FastSha1Data(state, in_buf, 64);
        memset(in_buf, 0, 64);
    }
    STORE_BE8(&in_buf[56], total_bit_len);
    FastSha1Data(state, in_buf, 64);
    BigEndian4Copy(state, hash_result, 5);
}

void Sha2_256(uint8_t *data_ptr, uint32_t data_len, uint8_t *hash_result)
{
    uint64_t total_bit_len;
    uint32_t num_whole_blks, whole_blks_len;
    uint8_t  in_buf[64], state[32];

    total_bit_len = 8 * data_len;
    memcpy(&state[0], SHA2_256_INIT, sizeof(state));

    // Do an even number of 64-byte SHA2_256 input data blocks:
    num_whole_blks = data_len / 64;
    if (num_whole_blks != 0)
    {
        whole_blks_len = 64 * num_whole_blks;
        FastSha2_256Data(state, data_ptr, whole_blks_len);
        data_ptr += whole_blks_len;
        data_len -= whole_blks_len;
    }

    // Now do the final blk or 2.  Note that data_len MUST be <= 63 here.
    memset(in_buf, 0, sizeof(in_buf));
    if (data_len != 0)
        memcpy(in_buf, data_ptr, data_len);

    in_buf[data_len] = 0x80;
    if (56 <= data_len)
    {
        FastSha2_256Data(state, in_buf, 64);
        memset(in_buf, 0, 64);
    }

    STORE_BE8(&in_buf[56], total_bit_len);
    FastSha2_256Data(state, in_buf, 64);
    BigEndian4Copy(state, hash_result, 8);
}

void BigEndian4Copy(void *in_ptr, void *out_ptr, uint32_t num_words)
{
    uint32_t *in32_ptr, *out32_ptr, idx;

    in32_ptr  = (uint32_t *) in_ptr;
    out32_ptr = (uint32_t *) out_ptr;
    for (idx = 0; idx < num_words; idx++)
        *out32_ptr++ = __builtin_bswap32(*in32_ptr++);
}

void print_sha(uint8_t *result, uint32_t len) {
   printf("Result: ");
   int x;
   for(x = 0; x < len; x++)
     printf("%02x", result[x]);
   putchar( '\n' );

}
