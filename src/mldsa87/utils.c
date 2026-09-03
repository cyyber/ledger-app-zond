#include "utils.h"
#include <string.h>
#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include "constant.h"
#include "cx.h"
#include "polyvec.h"
#include "globals.h"
#include "os_nvm.h"
#include "os_pic.h"
#include "rlp_decode.h"

static void byte_to_hex(uint8_t byte, char *out) {
    const char hex_chars[] = "0123456789abcdef";
    out[0] = hex_chars[(byte >> 4) & 0x0F];
    out[1] = hex_chars[byte & 0x0F];
}

static uint8_t hex_char_to_value(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0;
}

static uint8_t hex_string_to_byte(const char *hex) {
    return (hex_char_to_value(hex[0]) << 4 | hex_char_to_value(hex[1]));
}

void copy_array(uint8_t *dst, size_t dst_len, uint8_t *src, size_t src_len) {
    size_t len = dst_len < src_len ? dst_len : src_len;
    memmove(dst, src, len);
}

int decode_from_hex_string(const char *hex_str, uint8_t *out_bytes, size_t max_len) {
    size_t hex_len = strlen(hex_str);
    if (hex_len % 2 != 0) return -1;

    size_t byte_len = hex_len / 2;
    if (byte_len > max_len) return -1;

    char hex_val[2] = {0};

    for (size_t i = 0; i < byte_len; i++) {
        hex_val[0] = hex_str[i * 2];
        hex_val[1] = hex_str[i * 2 + 1];
        ;
        out_bytes[i] = hex_string_to_byte(hex_val);
    }
    return (int) byte_len;
}

void print_tx(zond_tx_t tx) {
    PRINTF("======== ZOND TX ========\n");
    PRINTF("Chain ID: 0x");
    for (int i = 0; i < tx.chain_id_len; i++) PRINTF("%02x", tx.chain_id[i]);
    PRINTF("\n");

    PRINTF("Nonce: 0x");
    for (int i = 0; i < tx.nonce_len; i++) PRINTF("%02x", tx.nonce[i]);
    PRINTF("\n");

    PRINTF("Gas Tip Cap: 0x");
    for (int i = 0; i < tx.gas_tip_cap_len; i++) PRINTF("%02x", tx.gas_tip_cap[i]);
    PRINTF("\n");

    PRINTF("Gas Fee Cap: 0x");
    for (int i = 0; i < tx.gas_fee_cap_len; i++) PRINTF("%02x", tx.gas_fee_cap[i]);
    PRINTF("\n");

    PRINTF("Gas: 0x");
    for (int i = 0; i < tx.gas_len; i++) PRINTF("%02x", tx.gas[i]);
    PRINTF("\n");

    PRINTF("To: 0x");
    for (int i = 0; i < 24; i++) PRINTF("%02x", tx.to[i]);
    PRINTF("\n");

    PRINTF("Value: 0x");
    for (int i = 0; i < tx.value_len; i++) PRINTF("%02x", tx.value[i]);
    PRINTF("\n");
    PRINTF("================\n");
}

void print_polyveck(PolyVecK *a) {
    PRINTF("\n");
    for (int i = 0; i < K; ++i) {
        PRINTF("[");
        for (int j = 0; j < N; ++j) {
            if (j == N - 1) {
                PRINTF("%d", a->vec[i].coeffs[j]);
            } else {
                PRINTF("%d,", a->vec[i].coeffs[j]);
            }
        }
        PRINTF("]");
        PRINTF("\n");
        PRINTF("\n");
        PRINTF("\n");
    }
    PRINTF("\n");
}

void print_polyvecl(PolyVecL *a) {
    PRINTF("\n");
    for (int i = 0; i < L; ++i) {
        PRINTF("[");
        for (int j = 0; j < N; ++j) {
            if (j == N - 1) {
                PRINTF("%d", a->vec[i].coeffs[j]);
            } else {
                PRINTF("%d,", a->vec[i].coeffs[j]);
            }
        }
        PRINTF("]");
        PRINTF("\n");
        PRINTF("\n");
        PRINTF("\n");
    }
    PRINTF("\n");
}

void print_hex(char *name, uint8_t *data, size_t len) {
    PRINTF("%s: ", name);
    for (int i = 0; i < len; i++) {
        PRINTF("%02x", data[i]);
    }
    PRINTF("\n");
}

void print_poly(Poly *a) {
    PRINTF("\n");
    PRINTF("[");
    for (int j = 0; j < N; ++j) {
        if (j == N - 1) {
            PRINTF("%d", a->coeffs[j]);
        } else {
            PRINTF("%d,", a->coeffs[j]);
        }
    }
    PRINTF("]");
    PRINTF("\n");
    PRINTF("\n");
}