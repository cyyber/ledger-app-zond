#ifndef MLDSA87_H
#define MLDSA87_H

#include "constant.h"
#include <stdint.h>
#include <stdbool.h>
#include "error.h"
#include <stdlib.h>

typedef struct
{
    uint8_t pk[CRYPTO_PUBLIC_KEY_BYTES];
    uint8_t sk[CRYPTO_SECRET_KEY_BYTES];
    uint8_t seed[SEED_BYTES];
    bool randomized_signing;
} MLDSA87;

ErrorCode new_mldsa87(MLDSA87 *d);
ErrorCode new_mldsa87_from_seed(uint8_t (*seed)[SEED_BYTES]);
ErrorCode new_mldsa87_from_hexseed(const char *hexseed, MLDSA87 *d);
void get_pk(MLDSA87 *d, uint8_t (*pk)[CRYPTO_PUBLIC_KEY_BYTES]);
void get_sk(MLDSA87 *d, uint8_t (*sk)[CRYPTO_SECRET_KEY_BYTES]);
void get_seed(MLDSA87 *d, uint8_t (*seed)[SEED_BYTES]);
const char *get_hexseed(MLDSA87 *d);
ErrorCode seal(MLDSA87 *d, uint8_t *ctx, size_t ctx_len, uint8_t *message, size_t message_len, uint8_t *s);
ErrorCode sign(MLDSA87 *d, uint8_t *ctx, size_t ctx_len, uint8_t *message, size_t message_len, uint8_t (*sig)[CRYPTO_BYTES]); 
uint8_t *open(uint8_t *ctx, size_t ctx_len, uint8_t *signatureMessage, size_t signatureMessage_len, uint8_t (*pk)[CRYPTO_PUBLIC_KEY_BYTES]);
bool verify(uint8_t *ctx, size_t ctx_len, uint8_t *message, size_t message_len, uint8_t signature[CRYPTO_BYTES], uint8_t (*pk)[CRYPTO_PUBLIC_KEY_BYTES]);
void extract_message(uint8_t *signatureMessage, size_t signatureMessage_len, uint8_t *msg);
void extract_signature(uint8_t *signatureMessage, uint8_t *sig); 

#endif