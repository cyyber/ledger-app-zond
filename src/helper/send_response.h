#pragma once

#include "os.h"

/**
 * Helper to send APDU response with the prefixed QRL address.
 *
 * response = 'Q' (1) ||
 *            G_context.address (64)
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int helper_send_response_address(void);

/**
 * Helper to send APDU response with signature and v (parity of
 * y-coordinate of R).
 *
 * response = N_storage.sig chunk
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int helper_send_response_sig(uint8_t index);

/**
 * Helper to send APDU response with public key chunk.
 * Public key is 2592 bytes, sent in 11 chunks:
 * - Chunks 0-9: 258 bytes each
 * - Chunk 10: 12 bytes (last)
 *
 * @param index - chunk index (0-10)
 * @return zero or positive integer if success, -1 otherwise.
 */
int helper_send_response_pk_chunk(uint8_t index);
