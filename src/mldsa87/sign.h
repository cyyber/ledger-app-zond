#ifndef SIGN_H
#define SIGN_H

#include "constant.h"
#include <stdint.h>
#include "error.h"
#include <stdbool.h>
#include <stdlib.h>
#include "polyvec.h"

ErrorCode crypto_sign_keypair(uint8_t (*seed)[SEED_BYTES]);

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