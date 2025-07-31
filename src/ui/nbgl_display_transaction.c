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

#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "os.h"
#include "glyphs.h"
#include "os_io_seproxyhal.h"
#include "nbgl_use_case.h"
#include "io.h"
#include "bip32.h"
#include "format.h"

#include "display.h"
#include "constants.h"
#include "globals.h"
#include "sw.h"
#include "address.h"
#include "validate.h"
#include "tx_types.h"
#include "menu.h"
#include "address.h"
#include "utils.h"

static char g_from_address[50];
static char g_amount[30];
static char g_to_address[50];
static char g_max_fees[30];

static nbgl_contentTagValue_t pairs[4];
static nbgl_contentTagValueList_t pairList;

#define MAX_DECIMAL_DIGITS 40
#define MAX_RESULT_LEN 50

static void uint8_array_to_decimal(const uint8_t *bytes, size_t len, char *out) {
    uint8_t temp[32] = {0};  // Ensure full zero-init
    memcpy(temp, bytes, len);

    char result[MAX_DECIMAL_DIGITS];
    int result_index = MAX_DECIMAL_DIGITS;

    // Repeated division by 10
    while (1) {
        int remainder = 0;
        int is_zero = 1;

        for (size_t i = 0; i < len; i++) {
            int val = (remainder << 8) | temp[i];
            temp[i] = val / 10;
            remainder = val % 10;
            if (temp[i] != 0)
                is_zero = 0;
        }

        result[--result_index] = '0' + remainder;
        if (is_zero) break;
    }

    size_t num_digits = MAX_DECIMAL_DIGITS - result_index;
    memcpy(out, &result[result_index], num_digits);
    out[num_digits] = '\0';  // Proper null termination
}

// Inserts decimal point `decimals` from the right, trims trailing zeros
static void format_with_decimals(const char *raw, int decimals, char *out) {
    size_t len = strlen(raw);

    if (decimals == 0) {
        strcpy(out, raw);
        return;
    }

    char buffer[MAX_RESULT_LEN];
    char *dot = buffer;

    if (len <= decimals) {
        strcpy(buffer, "0.");
        dot += 2;
        for (int i = 0; i < decimals - len; i++)
            *dot++ = '0';
        strcpy(dot, raw);
    } else {
        size_t int_part = len - decimals;
        strncpy(buffer, raw, int_part);
        buffer[int_part] = '.';
        strcpy(buffer + int_part + 1, raw + int_part);
    }

    // Trim trailing zeros
    char *end = buffer + strlen(buffer) - 1;
    while (*end == '0' && end > buffer) {
        *end-- = '\0';
    }
    if (*end == '.') *end = '\0';  // remove dot if nothing after

    strcpy(out, buffer);
}

// Wrapper: amount in wei to ETH
static void convert_amount_to_eth(const uint8_t *amount, size_t len, char *out_str) {
    char dec[MAX_RESULT_LEN];
    uint8_array_to_decimal(amount, len, dec);
    format_with_decimals(dec, 18, out_str);
}

// Wrapper: fees in wei to Gwei
static void convert_fees_to_gwei(const uint8_t *fees, size_t len, char *out_str) {
    char dec[MAX_RESULT_LEN];
    uint8_array_to_decimal(fees, len, dec);
    format_with_decimals(dec, 9, out_str);
}

// called when long press button on 3rd page is long-touched or when reject footer is touched
static void review_choice(bool confirm) {
    // Answer, display a status page and go back to main
    nbgl_useCaseSpinner("Signing");
    validate_transaction(confirm);
    if (confirm) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_SIGNED, ui_menu_main);
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_REJECTED, ui_menu_main);
    }
}

static void bytes_to_hex_string(uint8_t *byte, size_t byte_len, char *str) {
    const char hex_chars[] = "0123456789abcdef";

    for(int i = 0; i < byte_len; i++) {
        str[0 + i*2] = hex_chars[(byte[i] >> 4) & 0x0F];
        str[1 + i*2] = hex_chars[byte[i] & 0x0F];
    }
}

// Public function to start the transaction review
// - Check if the app is in the right state for transaction review
// - Format the amount and address strings in g_amount and g_address buffers
// - Display the first screen of the transaction review
// - Display a warning if the transaction is blind-signed
int ui_display_transaction_bs_choice(bool is_blind_signed, zond_tx_t *tx) {
    if (G_context.req_type != CONFIRM_TRANSACTION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }

    PRINTF("======== ZOND TX ========\n");
    PRINTF("Chain ID: 0x");
    for (int i = 0; i < tx->chain_id_len; i++) PRINTF("%02x", tx->chain_id[i]);
    PRINTF("\n");

    PRINTF("Nonce: 0x");
    for (int i = 0; i < tx->nonce_len; i++) PRINTF("%02x", tx->nonce[i]);
    PRINTF("\n");

    PRINTF("Gas Tip Cap: 0x");
    for (int i = 0; i < tx->gas_tip_cap_len; i++) PRINTF("%02x", tx->gas_tip_cap[i]);
    PRINTF("\n");

    PRINTF("Gas Fee Cap: 0x");
    for (int i = 0; i < tx->gas_fee_cap_len; i++) PRINTF("%02x", tx->gas_fee_cap[i]);
    PRINTF("\n");

    PRINTF("Gas: 0x");
    for (int i = 0; i < tx->gas_len; i++) PRINTF("%02x", tx->gas[i]);
    PRINTF("\n");

    PRINTF("To: 0x");
    for (int i = 0; i < ADDRESS_LENGTH; i++) PRINTF("%02x", tx->to[i]);
    PRINTF("\n");

    PRINTF("Value: 0x");
    for (int i = 0; i < tx->value_len; i++) PRINTF("%02x", tx->value[i]);
    PRINTF("\n");
    PRINTF("================\n");

    PRINTF("DERIVE ADDRESS START\n");
    nbgl_useCaseSpinner("Getting address");
    cx_err_t error = address_from_bip32_path(G_context.bip32_path,
                                                  G_context.bip32_path_len,
                                                  G_context.address);
    PRINTF("DERIVE ADDRESS END\n");
    PRINTF("from ");
    for(int i = 0; i < ADDRESS_SIZE; i++) {
        PRINTF("%02x", G_context.address[i]);
    }
    PRINTF("\n");

    //Format from address
    memset(g_from_address, 0, sizeof(g_from_address));
    char from_str[ADDRESS_SIZE*2+1] = {0}; 
    bytes_to_hex_string(G_context.address, ADDRESS_SIZE, from_str);
    strncpy(g_from_address + 1, from_str, sizeof(g_from_address)-1);
    g_from_address[0] = 'Z';

    // Format amount
    char amount[30] = {0};
    memset(amount, 0, sizeof(amount));
    convert_amount_to_eth(tx->value, tx->value_len, amount);
    PRINTF("amount %s\n", amount);
    memset(g_amount, 0, sizeof(g_amount));
    snprintf(g_amount, sizeof(g_amount), "ZND %.*s", sizeof(amount), amount);

    //Format to address
    memset(g_to_address, 0, sizeof(g_to_address));
    char to_str[ADDRESS_LENGTH*2+1] = {0}; 
    bytes_to_hex_string(tx->to, ADDRESS_LENGTH, to_str);
    PRINTF("to %s\n", to_str);
    strncpy(g_to_address + 1, to_str, sizeof(g_to_address)-1);
    g_to_address[0] = 'Z';

    // Format max_fees
    char max_fees[30] = {0};
    memset(max_fees, 0, sizeof(max_fees));
    convert_amount_to_eth(tx->gas_fee_cap, tx->gas_fee_cap_len, max_fees);
    PRINTF("max fees %s\n", max_fees);
    memset(g_max_fees, 0, sizeof(g_max_fees));
    snprintf(g_max_fees, sizeof(g_max_fees), "ZND %.*s", sizeof(max_fees), max_fees);

    // Setup data to display
    pairs[0].item = "From";
    pairs[0].value = g_from_address;
    pairs[1].item = "Amount";
    pairs[1].value = g_amount;
    pairs[2].item = "To";
    pairs[2].value = g_to_address;
    pairs[3].item = "Max fees";
    pairs[3].value = g_max_fees;

    // Setup list
    pairList.nbMaxLinesForValue = 0;
    pairList.nbPairs = 4;
    pairList.pairs = pairs;

    if (is_blind_signed) {
        // Start blind-signing review flow
        nbgl_useCaseReviewBlindSigning(TYPE_TRANSACTION,
                                       &pairList,
                                       &ICON_APP_BOILERPLATE,
                                       "Review transaction\n",
                                       NULL,
#ifdef SCREEN_SIZE_WALLET
                                       "Sign transaction\n",
#else
                                       NULL,
#endif
                                       NULL,
                                       review_choice);
    } else {
        // Start review flow
        nbgl_useCaseReview(TYPE_TRANSACTION,
                           &pairList,
                           &ICON_APP_BOILERPLATE,
                           "Review transaction\n",
                           NULL,
#ifdef SCREEN_SIZE_WALLET
                           "Sign transaction\n",
#else
                           NULL,
#endif
                           review_choice);
    }
    return 0;
}

// Flow used to display a blind-signed transaction
int ui_display_blind_signed_transaction(void) {
    return ui_display_transaction_bs_choice(true, NULL);
}

// Flow used to display a clear-signed transaction
int ui_display_transaction(zond_tx_t *tx) {
    return ui_display_transaction_bs_choice(false, tx);
}
