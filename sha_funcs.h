#ifndef FNCTIONS_SHA_INCLUDED
#define FUNCTIONS_SHA_INCLUDED

void Sha1(uint8_t *data_ptr, uint32_t data_len, uint8_t *hash_result);
void Sha2_256(uint8_t *data_ptr, uint32_t data_len, uint8_t *hash_result);
void BigEndian4Copy(void *in_ptr, void *out_ptr, uint32_t num_words);

#endif
