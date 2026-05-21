#pragma once

#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include "address_constants.h"

/**
 * Derive a 64-byte QRL address from a BIP32 path.
 *
 * address = SHAKE256(ML-DSA-87 descriptor || ML-DSA-87 public key, 64 bytes)
 *
 * @param[in]  bip32_path
 *   BIP32 path used to derive the ML-DSA-87 seed.
 * @param[in]  bip32_path_len
 *   Number of path elements.
 * @param[out] out
 *   Pointer to output byte buffer for address.
 *
 * @return CX_OK if success, an error code otherwise.
 *
 */
cx_err_t address_from_bip32_path(const uint32_t bip32_path[], size_t bip32_path_len, uint8_t address[ADDRESS_SIZE]);
