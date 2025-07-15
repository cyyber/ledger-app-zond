#include <stdint.h>
#include "shake128.h"
#include <stdlib.h>
#include <string.h>

// Rotate left 64-bit
static inline uint64_t ROL64_128(uint64_t x, int n) {
    return (x << n) | (x >> (64 - n));
}

// Keccak-f[1600], 24 rounds
static void keccakf_128(uint64_t s[25]) {
    const uint64_t RC[24] = {
        0x0000000000000001ULL, 0x0000000000008082ULL,
        0x800000000000808AULL, 0x8000000080008000ULL,
        0x000000000000808BULL, 0x0000000080000001ULL,
        0x8000000080008081ULL, 0x8000000000008009ULL,
        0x000000000000008AULL, 0x0000000000000088ULL,
        0x0000000080008009ULL, 0x000000008000000AULL,
        0x000000008000808BULL, 0x800000000000008BULL,
        0x8000000000008089ULL, 0x8000000000008003ULL,
        0x8000000000008002ULL, 0x8000000000000080ULL,
        0x000000000000800AULL, 0x800000008000000AULL,
        0x8000000080008081ULL, 0x8000000000008080ULL,
        0x0000000080000001ULL, 0x8000000080008008ULL
    };
    const int r[25] = {
         0,  1, 62, 28, 27,
        36, 44,  6, 55, 20,
         3, 10, 43, 25, 39,
        41, 45, 15, 21,  8,
        18,  2, 61, 56, 14
    };
    for (int round = 0; round < 24; round++) {
        uint64_t C[5];
        uint64_t D[5];
        uint64_t B[25];
        for (int x = 0; x < 5; x++)
            C[x] = s[x] ^ s[x+5] ^ s[x+10] ^ s[x+15] ^ s[x+20];
        for (int x = 0; x < 5; x++)
            D[x] = C[(x+4)%5] ^ ROL64_128(C[(x+1)%5], 1);
        for (int i = 0; i < 25; i++)
            s[i] ^= D[i % 5];
        for (int i = 0; i < 25; i++) {
            int x = i % 5, y = i / 5;
            B[y + 5*((2*x + 3*y) % 5)] = ROL64_128(s[i], r[i]);
        }
        for (int i = 0; i < 25; i++)
            s[i] = B[i] ^ ((~B[(i+1)%5 + 5*(i/5)]) & B[(i+2)%5 + 5*(i/5)]);
        s[0] ^= RC[round];
    }
}

void shake128_init(shake128_ctx *ctx) {
    MEMSET(ctx, 0, sizeof(*ctx));
}

void shake128_absorb(shake128_ctx *ctx, const uint8_t *in, size_t inlen) {
    while (inlen--) {
        size_t i = ctx->pos++;
        ctx->s[i/8] ^= (uint64_t)(*in++) << (8 * (i % 8));
        if (ctx->pos == 168) {
            keccakf_128(ctx->s);
            ctx->pos = 0;
        }
    }
}

void shake128_finalize(shake128_ctx *ctx) {
    ctx->s[ctx->pos / 8] ^= (uint64_t)0x1F << (8 * (ctx->pos % 8));
    ctx->s[(168 - 1) / 8] ^= (uint64_t)0x80 << (8 * ((168 - 1) % 8));
    keccakf_128(ctx->s);
    ctx->pos = 0;
    ctx->squeezing = 1;
}

void shake128_squeeze(shake128_ctx *ctx, uint8_t *out, size_t outlen) {
    if (!ctx->squeezing) shake128_finalize(ctx);
    while (outlen--) {
        *out++ = (ctx->s[ctx->pos / 8] >> (8 * (ctx->pos % 8))) & 0xFF;
        if (++ctx->pos == 168) {
            keccakf_128(ctx->s);
            ctx->pos = 0;
        }
    }
}

void shake128_clear(shake128_ctx *ctx) {
    MEMSET(ctx, 0, sizeof(*ctx));
}
