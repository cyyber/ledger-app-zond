#include "mldsa87.h"
#include "sign.h"
#include "utils.h"
#include "cx.h"
#include <string.h>
#include <stdarg.h>
#include "globals.h"
#include "os_pic.h"

ErrorCode new_mldsa87(MLDSA87 *d) {
    uint8_t sk[CRYPTO_SECRET_KEY_BYTES] = {0};

    uint8_t pk[CRYPTO_PUBLIC_KEY_BYTES] = {0};

    uint8_t seed[SEED_BYTES] = {0};

    cx_rng(seed, sizeof(seed));
    int err = crypto_sign_keypair(&seed);
    if (err != 0) {
        d = NULL;
        return err;
    }

    memmove(d->pk, pk, sizeof(d->pk));
    memmove(d->sk, sk, sizeof(d->sk));
    memmove(d->seed, seed, sizeof(d->seed));
    d->randomized_signing = false;

    return ERR_NONE;
}

ErrorCode new_mldsa87_from_seed(uint8_t (*seed)[SEED_BYTES]) {
    int err = 0;
    err = crypto_sign_keypair(seed);
    if (err != 0) {
        return err;
    }
    return ERR_NONE;
}
