#ifndef SIGN_H
#define SIGN_H

#include "constant.h"
#include <stdint.h>
#include "error.h"
#include <stdbool.h>
#include <stdlib.h>
#include "polyvec.h"

ErrorCode crypto_sign_keypair(uint8_t (*seed)[SEED_BYTES]);
ErrorCode crypto_sign_signature_internal(uint8_t *sig,
                                         size_t sig_len,
                                         uint8_t *m,
                                         size_t m_len,
                                         uint8_t *pre,
                                         size_t pre_len,
                                         uint8_t rnd[RND_BYTES],
                                         uint8_t (*sk)[CRYPTO_SECRET_KEY_BYTES]);
ErrorCode crypto_sign_signature(uint8_t *sig,
                                size_t sig_len,
                                uint8_t *m,
                                size_t m_len,
                                uint8_t *ctx,
                                size_t ctx_len,
                                uint8_t (*sk)[CRYPTO_SECRET_KEY_BYTES],
                                bool randomized_signing);
ErrorCode crypto_sign(uint8_t *msg,
                      size_t msg_len,
                      uint8_t *ctx,
                      size_t ctx_len,
                      uint8_t (*sk)[CRYPTO_SECRET_KEY_BYTES],
                      bool randomizedSigning,
                      uint8_t *sig);
ErrorCode crypto_sign_verify_internal(uint8_t sig[CRYPTO_BYTES],
                                      uint8_t *m[],
                                      size_t m_len,
                                      uint8_t *pre,
                                      size_t pre_len,
                                      uint8_t (*pk)[CRYPTO_PUBLIC_KEY_BYTES],
                                      bool *ret);
ErrorCode crypto_sign_verify(uint8_t sig[CRYPTO_BYTES],
                             uint8_t *m,
                             size_t m_len,
                             uint8_t *ctx,
                             size_t ctx_len,
                             uint8_t (*pk)[CRYPTO_PUBLIC_KEY_BYTES],
                             bool *ret);
ErrorCode crypto_sign_open(uint8_t *sm,
                           size_t sm_len,
                           uint8_t *ctx,
                           size_t ctx_len,
                           uint8_t (*pk)[CRYPTO_PUBLIC_KEY_BYTES],
                           uint8_t *sig);

ErrorCode crypto_sign_optimized(const uint32_t bip32_path[],
                                size_t bip32_path_len,
                                uint8_t *message,
                                size_t message_len);
ErrorCode crypto_verify_optimized(const uint32_t bip32_path[],
                                  size_t bip32_path_len,
                                  uint8_t *sig,
                                  size_t sig_len,
                                  uint8_t *message,
                                  size_t message_len,
                                  bool *is_verified);

/**
 * Zero the NVM regions holding secret signing state (y, t0, w0, sampling
 * buffer). Call after every sign/verify operation, success or failure.
 */
void wipe_nvm_secrets(void);
#endif