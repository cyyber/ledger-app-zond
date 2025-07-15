#pragma once

#include <stdint.h>

#include "ux.h"

#include "io.h"
#include "types.h"
#include "constants.h"
#include "polyvec.h"
#include "poly.h"

/**
 * Global context for user requests.
 */
extern global_ctx_t G_context;

/**
 * Global structure for NVM data storage.
 */
typedef struct internal_storage_t {
    PolyVecK t1;
    PolyVecK w1;
    PolyVecK w0;
    PolyVecK h;
    PolyVecL y;
    PolyVecL z;
    Poly cp;
    uint8_t address_hash_input[(DESCRIPTOR_BYTES + CRYPTO_PUBLIC_KEY_BYTES) * 2];
    uint8_t sig[CRYPTO_BYTES];
    uint8_t pk[CRYPTO_PUBLIC_KEY_BYTES];
    uint8_t dummy1_allowed;
    uint8_t dummy2_allowed;
    uint8_t initialized;
} internal_storage_t;

extern const internal_storage_t N_storage_real;
#define N_storage (*(volatile internal_storage_t *) PIC(&N_storage_real))


