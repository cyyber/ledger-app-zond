/*****************************************************************************
 *   Ledger App Boilerplate.
 *   (c) 2020 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <string.h>   // memmove

#include "os.h"
#include "cx.h"
#include "ledger_assert.h"

#include "address.h"

#include "tx_types.h"
#include "constant.h"
#include "mldsa87.h"
#include "shake256.h"
#include "globals.h"
#include "utils.h"

bool address_from_pubkey(const uint8_t public_key[static 65], uint8_t *out, size_t out_len) {
    uint8_t address[32] = {0};

    LEDGER_ASSERT(out != NULL, "NULL out");

    if (out_len < ADDRESS_LEN) {
        return false;
    }

    if (cx_keccak_256_hash(public_key + 1, 64, address) != CX_OK) {
        return false;
    }

    memmove(out, address + sizeof(address) - ADDRESS_LEN, ADDRESS_LEN);

    return true;
}

cx_err_t address_from_bip32_path(const uint32_t bip32_path[], size_t bip32_path_len, uint8_t address[ADDRESS_SIZE]) {
    uint8_t raw_seed[64] = {0};
    cx_err_t err = os_derive_bip32_no_throw(
        CX_CURVE_SECP256K1,
        bip32_path,
        bip32_path_len,
        raw_seed,
        NULL
    );
    if(err != 0) {
        return -1;
    }
    uint8_t mldsa87_seed[32] = {0};
    for(int i = 0; i < 32; i++) {
        mldsa87_seed[i] = raw_seed[i];
    }

    new_mldsa87_from_seed(&mldsa87_seed);

    static uint8_t input[(DESCRIPTOR_BYTES + CRYPTO_PUBLIC_KEY_BYTES)*2] = {0};
    explicit_bzero(input, (DESCRIPTOR_BYTES + CRYPTO_PUBLIC_KEY_BYTES)*2);
    input[0 + DESCRIPTOR_BYTES + CRYPTO_PUBLIC_KEY_BYTES] = 1;
    input[1 + DESCRIPTOR_BYTES + CRYPTO_PUBLIC_KEY_BYTES] = 0;
    input[2 + DESCRIPTOR_BYTES + CRYPTO_PUBLIC_KEY_BYTES] = 0;
    for(int i = 0; i < CRYPTO_PUBLIC_KEY_BYTES; ++i) {
        input[i + DESCRIPTOR_BYTES + DESCRIPTOR_BYTES + CRYPTO_PUBLIC_KEY_BYTES] = N_storage.pk[i];
    }

    uint8_t output[32] = {0};
    shake256_ctx ctx;
    shake256_init(&ctx);
    shake256_absorb(&ctx, input, (DESCRIPTOR_BYTES + CRYPTO_PUBLIC_KEY_BYTES) * 2);
    shake256_finalize(&ctx);
    shake256_squeeze(&ctx, output, 32);
    shake256_clear(&ctx);

    for(int i = 0; i < ADDRESS_SIZE; i++) {
        address[i] = output[i + (32-ADDRESS_SIZE)];
    }
    return 0;
}
