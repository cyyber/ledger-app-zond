#pragma once

#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include "constant.h"

/**
 * Convert public key to address.
 *
 * address = zero-left-padded Keccak256(public_key) (48 bytes)
 *
 * @param[in]  public_key
 *   Pointer to byte buffer with public key.
 *   The public key is represented as 65 bytes with 1 byte for format and 32 bytes for
 *   each coordinate.
 * @param[out] out
 *   Pointer to output byte buffer for address.
 * @param[in]  out_len
 *   Length of output byte buffer.
 *
 * @return true if success, false otherwise.
 *
 */
bool address_from_pubkey(const uint8_t public_key[static 65], uint8_t *out, size_t out_len);


cx_err_t address_from_bip32_path(const uint32_t bip32_path[], size_t bip32_path_len, uint8_t address[ADDRESS_SIZE]);
