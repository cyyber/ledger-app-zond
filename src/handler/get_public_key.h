#pragma once

#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <stdint.h>   // uint*_t

#include "buffer.h"

#include "types.h"

/**
 * Handler for GET_PUBLIC_KEY command. If successfully parse BIP32 path,
 * derive the ML-DSA-87 address and send APDU response.
 *
 * @param[in,out] cdata
 *   Command data with BIP32 path.
 * @param[in]     display
 *   Whether to display address on screen or not.
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handler_get_public_key(buffer_t *cdata, bool display);

/**
 * Handler for fetching public key chunks.
 * Must be called after handler_get_public_key to ensure pk is derived.
 *
 * @param[in] chunk_index
 *   Chunk index (0-10). Total 11 chunks for 2592 byte public key.
 *
 * @return zero or positive integer if success, negative integer otherwise.
 */
int handler_get_public_key_chunk(uint8_t chunk_index);
