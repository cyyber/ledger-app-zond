#ifndef PACKING_H
#define PACKING_H

#include "constant.h"
#include "polyvec.h"

void pack_pk(uint8_t (*pkb)[CRYPTO_PUBLIC_KEY_BYTES], uint8_t rho[SEED_BYTES], PolyVecK *t1);
void unpack_pk(uint8_t (*rho)[SEED_BYTES], PolyVecK *t1, uint8_t (*pkb)[CRYPTO_PUBLIC_KEY_BYTES]);
void pack_sk(uint8_t rho[SEED_BYTES],
             uint8_t tr[TR_BYTES],
             uint8_t key[SEED_BYTES],
             PolyVecK *t0,
             PolyVecL *s1,
             PolyVecK *s2);
void unpack_sk(uint8_t (*rho)[SEED_BYTES],
               uint8_t (*tr)[TR_BYTES],
               uint8_t (*key)[SEED_BYTES],
               PolyVecK *t0,
               PolyVecL *s1,
               PolyVecK *s2,
               uint8_t (*skb)[CRYPTO_SECRET_KEY_BYTES]);
ErrorCode pack_sig(uint8_t sigb[],
                   size_t sigb_len,
                   uint8_t c[CTILDE_BYTES],
                   PolyVecL *z,
                   PolyVecK *h);
int32_t unpack_sig(uint8_t (*c)[CTILDE_BYTES],
                   PolyVecL *z,
                   PolyVecK *h,
                   uint8_t sigBytes[CRYPTO_BYTES]);

#endif  // !PACKING_H
