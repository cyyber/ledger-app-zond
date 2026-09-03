#ifndef MLDSA87_H
#define MLDSA87_H

#include "constant.h"
#include <stdint.h>
#include <stdbool.h>
#include "error.h"
#include <stdlib.h>

typedef struct {
    uint8_t pk[CRYPTO_PUBLIC_KEY_BYTES];
    uint8_t sk[CRYPTO_SECRET_KEY_BYTES];
    uint8_t seed[SEED_BYTES];
    bool randomized_signing;
} MLDSA87;

ErrorCode new_mldsa87(MLDSA87 *d);
ErrorCode new_mldsa87_from_seed(uint8_t (*seed)[SEED_BYTES]);
void extract_signature(uint8_t *signatureMessage, uint8_t *sig);

#endif