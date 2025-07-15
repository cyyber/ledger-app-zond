#include "mldsa87.h"
#include "sign.h"
#include "utils.h"
#include "cx.h"
#include <string.h>
#include <stdarg.h>
#include "globals.h"
#include "os_pic.h"

ErrorCode new_mldsa87(MLDSA87 *d) {
    uint8_t sk[CRYPTO_SECRET_KEY_BYTES] = {0};

	uint8_t pk[CRYPTO_PUBLIC_KEY_BYTES] = {0};

	uint8_t seed[SEED_BYTES] = {0};

	cx_rng(seed, sizeof(seed));
    int err = crypto_sign_keypair(&seed); 
	if(err != 0) {
        d = NULL; 
		return err;
	}

    memmove(d->pk, pk, sizeof(d->pk));
    memmove(d->sk, sk, sizeof(d->sk));
    memmove(d->seed, seed, sizeof(d->seed));
    d->randomized_signing=false;

	return ERR_NONE;
}

ErrorCode new_mldsa87_from_seed(uint8_t (*seed)[SEED_BYTES]) {
    int err = 0;
	err = crypto_sign_keypair(seed); 
	if(err != 0) {
		return err;
	}
	return ERR_NONE;
}

ErrorCode new_mldsa87_from_hexseed(const char *hexseed, MLDSA87 *d) {
    size_t len = strlen(hexseed)/2;
    uint8_t out[SEED_BYTES] = {0};
    int err = decode_from_hex_string(hexseed, out, len);
	if(err < 0) {
		return err;
	}
	uint8_t seed[SEED_BYTES] = {0};
	copy_array(seed, SEED_BYTES, out, SEED_BYTES);
	return new_mldsa87_from_seed(&seed);
}

void get_pk(MLDSA87 *d, uint8_t (*pk)[CRYPTO_PUBLIC_KEY_BYTES]) {
    for(size_t i = 0; i < CRYPTO_PUBLIC_KEY_BYTES; ++i) {
        (*pk)[i] = d->pk[i];
    }
}
void get_sk(MLDSA87 *d, uint8_t (*sk)[CRYPTO_SECRET_KEY_BYTES]) {
    for(size_t i = 0; i < CRYPTO_SECRET_KEY_BYTES; ++i) {
        (*sk)[i] = d->sk[i];
    }
}
void get_seed(MLDSA87 *d, uint8_t (*seed)[SEED_BYTES]) {
    for(size_t i = 0; i < SEED_BYTES; ++i) {
        (*seed)[i] = d->seed[i];
    }
}
const char *get_hexseed(MLDSA87 *d) {
    uint8_t *seed = d->seed;
    char *hexseed = "abcdef";
    uint8_t buf[SEED_BYTES*2 + 2 + 1] = {0};
    buf[0] = '0';
    buf[1] = 'x';
    memmove(buf + 2, hexseed, strlen(hexseed));
    buf[2 + strlen(hexseed)] = '\0';  // null terminate
    return (char *)buf;
}

ErrorCode seal(MLDSA87 *d, uint8_t *ctx, size_t ctx_len, uint8_t *message, size_t message_len, uint8_t *s) {
    return crypto_sign(message, message_len, ctx, ctx_len, &d->sk, d->randomized_signing, s);
}

ErrorCode sign(MLDSA87 *d, uint8_t *ctx, size_t ctx_len, uint8_t *message, size_t message_len, uint8_t (*sig)[CRYPTO_BYTES]) {
    uint8_t sm[CRYPTO_BYTES + MESSAGE_LEN] = {0};
    int err = crypto_sign(message, message_len, ctx, ctx_len, &d->sk, d->randomized_signing, sm);
	if(err == 0) {
		copy_array((uint8_t*)sig, CRYPTO_BYTES, sm, CRYPTO_BYTES);
	}
    return err;
}

uint8_t *open(uint8_t *ctx, size_t ctx_len, uint8_t *signatureMessage, size_t signatureMessage_len, uint8_t (*pk)[CRYPTO_PUBLIC_KEY_BYTES]) {
    uint8_t *sig = NULL;
    crypto_sign_open(signatureMessage, signatureMessage_len, ctx, ctx_len, pk, sig);
	return sig;
}

bool verify(uint8_t *ctx, size_t ctx_len, uint8_t *message, size_t message_len, uint8_t signature[CRYPTO_BYTES], uint8_t (*pk)[CRYPTO_PUBLIC_KEY_BYTES]) {
    bool result = false;
    int err = crypto_sign_verify(signature, message, message_len, ctx, ctx_len, pk, &result); 
	if(err != 0) {
		return result;
	}
	return result;
}

void extract_message(uint8_t *signatureMessage, size_t signatureMessage_len, uint8_t *msg) {
    copy_array(msg, signatureMessage_len - CRYPTO_BYTES, signatureMessage + CRYPTO_BYTES, signatureMessage_len - CRYPTO_BYTES);
}
void extract_signature(uint8_t *signatureMessage, uint8_t *sig) {
    copy_array(sig, CRYPTO_BYTES, signatureMessage, CRYPTO_BYTES);
}