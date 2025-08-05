#include "sign.h"
#include "packing.h"
#include <string.h>
#include "utils.h"
#include "cx.h"
#include "shake128.h"
#include "shake256.h"
#include "polyvec.h"
#include "os_nvm.h"
#include "os_pic.h"
#include "globals.h"
#include "os.h"
#include <inttypes.h>

typedef union {
	Poly temp_poly;
	uint8_t buf_squeeze[850];
} u_temp_poly_buffer_squeeze;

u_temp_poly_buffer_squeeze ubuf;

void shake128_squeeze_volatile(shake128_ctx *ctx, volatile uint8_t *out, size_t offset, size_t outlen) {
    uint32_t counter = 0;
	if (!ctx->squeezing) shake128_finalize(ctx);
	uint8_t temp = 0;
	explicit_bzero(&ubuf.buf_squeeze, sizeof(ubuf.buf_squeeze));
    for(int i = 0; i < outlen; i++) {
		temp = (ctx->s[ctx->pos / 8] >> (8 * (ctx->pos % 8))) & 0xFF;
		ubuf.buf_squeeze[i] = temp;
		// nvm_write((void *)&out[i+offset], &temp, sizeof(uint8_t));counter++;
        // *out++ = (ctx->s[ctx->pos / 8] >> (8 * (ctx->pos % 8))) & 0xFF;
        if (++ctx->pos == 168) {
            keccakf_128(ctx->s);
            ctx->pos = 0;
        }
    }
	nvm_write((void *)&out[offset], &ubuf.buf_squeeze, sizeof(uint8_t)*outlen);counter++;
	PRINTF("shake128_squeeze_volatile %u\n", counter);
}

static void shake256_squeeze_volatile(shake256_ctx *ctx, volatile uint8_t *out, size_t outlen) {
	uint32_t counter = 0;
    if (!ctx->squeezing) shake256_finalize(ctx);
	uint8_t temp = 0;
	// uint8_t temp[CTILDE_BYTES] = {0};
	explicit_bzero(&ubuf.buf_squeeze, sizeof(ubuf.buf_squeeze));
	for(int i = 0; i < outlen; i++) {
		temp = (ctx->s[ctx->pos / 8] >> (8 * (ctx->pos % 8))) & 0xFF;
		ubuf.buf_squeeze[i] = temp;
		// nvm_write((void*)&out[i], &temp, sizeof(uint8_t));counter++;
        if (++ctx->pos == SHAKE256_RATE) {
            keccakf_256(ctx->s);
            ctx->pos = 0;
        }
	}
	nvm_write((void *)&out[0], &ubuf.buf_squeeze[0], outlen);counter++;
	PRINTF("shake256_squeeze_volatile %u\n", counter);
}

static void combined_method(volatile PolyVecK *t1, PolyVecL *s1hat, uint8_t rho[SEED_BYTES]) {
	uint32_t counter = 0;

    size_t a_len = N;
    size_t buf_len = POLY_UNIFORM_N_BLOCKS*STREAM_128_BLOCK_BYTES;
    uint32_t aLen = (uint32_t)(a_len);
	uint32_t bufLen = (uint32_t)(buf_len);
    int32_t poly_buffer[256] = {0};
    Poly t_poly;
	int32_t temp = 0;
	for(int i = 0; i < K; i++) {
		// memmove(&ubuf.temp_poly, &t1->vec[i], sizeof(Poly));
		explicit_bzero(&t_poly, sizeof(t_poly));
		for(int j = 0; j < L; j++) {
			explicit_bzero(poly_buffer, sizeof(poly_buffer));
			// uint8_t buf[POLY_UNIFORM_N_BLOCKS*STREAM_128_BLOCK_BYTES + 2] = {0};

			shake128_ctx ctx;
			shake128_init(&ctx);
			shake128_absorb(&ctx, rho, SEED_BYTES);
            uint16_t nonce = ((uint16_t)i<<8)+(uint16_t)j;
			uint8_t nonces[2] = {(uint8_t)(nonce), (uint8_t)(nonce >> 8)};
			shake128_absorb(&ctx, nonces, 2);
			shake128_finalize(&ctx);
			shake128_squeeze_volatile(&ctx, N_storage.buf, 0, POLY_UNIFORM_N_BLOCKS*STREAM_128_BLOCK_BYTES + 2);

			a_len = N;
			buf_len = POLY_UNIFORM_N_BLOCKS*STREAM_128_BLOCK_BYTES + 2;
			uint32_t ctr = 0, pos = 0, t = 0;
			while(ctr < a_len && pos+3 <= buf_len) {
				t = (uint32_t)(N_storage.buf[pos]);
				t |= (uint32_t)(N_storage.buf[pos+1]) << 8;
				t |= (uint32_t)(N_storage.buf[pos+2]) << 16;
				t &= 0x7fffff;

				pos += 3;

				if(t < Q_CONST) {
					poly_buffer[ctr] = (int32_t)t;
					ctr++;
				}
			}

			bufLen = POLY_UNIFORM_N_BLOCKS * STREAM_128_BLOCK_BYTES;
			uint8_t temp_val = 0;
			while(ctr < N) {
				size_t off = bufLen % 3;
				for(size_t n = 0; n < off; n++) {\
					temp_val = N_storage.buf[bufLen-off+n];
					nvm_write((void *)&N_storage.buf[n], &temp_val, sizeof(uint8_t));counter++;
					// buf[n] = buf[bufLen-off+n];
				}
				shake128_squeeze_volatile(&ctx, N_storage.buf, (size_t)off, STREAM_128_BLOCK_BYTES);
                aLen = N-ctr;
				bufLen = STREAM_128_BLOCK_BYTES + off;
				uint32_t ctrNew = 0;
				pos = 0, t = 0;
				while(ctrNew < aLen && pos+3 <= bufLen) {
					t = (uint32_t)(N_storage.buf[pos]);
					t |= (uint32_t)(N_storage.buf[pos+1]) << 8;
					t |= (uint32_t)(N_storage.buf[pos+2]) << 16;
					t &= 0x7fffff;

					pos += 3;

					if(t < Q_CONST) {
						poly_buffer[ctrNew + ctr] = (int32_t)t;
						ctrNew++;
					}
				}
				ctr += ctrNew;
			}
			shake128_clear(&ctx);

			memmove(&ubuf.temp_poly, &t1->vec[i], sizeof(Poly));
            if(j == 0) {
                for(int k = 0; k < N; k++) {
                    int32_t t2 = 0;
                    int64_t a = ((int64_t)(poly_buffer[k]) * (int64_t)(s1hat->vec[j].coeffs[k]));
                    t2 = (int32_t) ((int64_t)(int32_t)a * Q_INV);
                    t2 = (int32_t) ((a - (int64_t)t2*Q_CONST) >> 32);
					ubuf.temp_poly.coeffs[k] = t2;
					// nvm_write((void*)&t1->vec[i].coeffs[k], &t2, sizeof(int32_t));counter++;
					
                }
            } else {				
                for(int k = 0; k < N; k++) {
                    int32_t t3 = 0;
                    int64_t a = ((int64_t)(poly_buffer[k]) * (int64_t)(s1hat->vec[j].coeffs[k]));
                    t3 = (int32_t) ((int64_t)(int32_t)a * Q_INV);
                    t3 = (int32_t) ((a - (int64_t)t3*Q_CONST) >> 32);
                    t_poly.coeffs[k] = t3;
                }
                for(int k = 0; k < N; k++) {
					// temp = t1->vec[i].coeffs[k] + t_poly.coeffs[k];
					ubuf.temp_poly.coeffs[k] = ubuf.temp_poly.coeffs[k] + t_poly.coeffs[k];
					// counter++;
					// nvm_write((void*)&t1->vec[i].coeffs[k], &temp, sizeof(int32_t));
					
                }
            }
			nvm_write((void*)&t1->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
		}
		// nvm_write((void*)&t1->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("combined_method %d\n", counter);
}

typedef union {
	PolyVecL s1hat;
	PolyVecK t0;
} union_s1hat_t0;

static void poly_vec_k_reduce_volatile(volatile PolyVecK *v) {
	uint32_t counter = 0;

    for(int i = 0; i < K; ++i) {
		memmove(&ubuf.temp_poly, &v->vec[i], sizeof(Poly));
		for(int j = 0; j < N; ++j) {
			int32_t t = 0;
			int32_t a =  v->vec[i].coeffs[j];

			t = (a + (1 << 22)) >> 23;
			t = a - t*Q_CONST;
			ubuf.temp_poly.coeffs[j] = t;

		}
		nvm_write((void*)&v->vec[i], (void *)&ubuf.temp_poly, sizeof(Poly));counter++;
	}

	PRINTF("poly_vec_k_reduce_volatile %d\n", counter);
}

static void poly_vec_k_inv_ntt_to_mont_volatile(volatile PolyVecK *v) {
	uint32_t counter = 0;

    for(int i = 0; i < K; i++) {
		memmove(&ubuf.temp_poly, &v->vec[i], sizeof(Poly));
		uint32_t count = 0, start = 0, j = 0, k = 0;
		int32_t zeta = 0, t = 0;
		int32_t f = (int32_t)41978;

		k = 256;

		int32_t temp = 0;

		for(count = 1; count < N; count <<= 1) {
			for(start = 0; start < N; start = (j + count)) {
				k--;
				zeta = -ZETAS[k];
				for(j = start; j < (start+count); j++) {
					t = ubuf.temp_poly.coeffs[j];
					
					temp = t + ubuf.temp_poly.coeffs[j+count];
					ubuf.temp_poly.coeffs[j] = temp;
					// nvm_write((void*)&v->vec[i].coeffs[j], &temp, sizeof(int32_t));counter++;
					
					temp = t - ubuf.temp_poly.coeffs[j+count];
					ubuf.temp_poly.coeffs[j+count] = temp;
					// nvm_write((void*)&v->vec[i].coeffs[j+count], &temp, sizeof(int32_t));counter++;
					int32_t t2 = 0;
					int64_t a = (int64_t)zeta * (int64_t)ubuf.temp_poly.coeffs[j+count];
					t2 = (int32_t) ((int64_t)(int32_t)a * Q_INV);
					t2 = (int32_t) ((a - (int64_t)t2*Q_CONST) >> 32);
					ubuf.temp_poly.coeffs[j+count] = t2;
					// nvm_write((void*)&v->vec[i].coeffs[j+count], &t2, sizeof(int32_t));counter++;
				}
			}
		}

		for(j = 0; j < N; ++j) {
			int32_t t3 = 0;
			int64_t a = (int64_t)f * (int64_t)ubuf.temp_poly.coeffs[j];
			t3 = (int32_t) ((int64_t)(int32_t)a * Q_INV);
			t3 = (int32_t) ((a - (int64_t)t3*Q_CONST) >> 32);
			ubuf.temp_poly.coeffs[j] = t3;
			// nvm_write((void*)&v->vec[i].coeffs[j], &t3, sizeof(int32_t));counter++;
		}
		nvm_write((void*)&v->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}

	PRINTF("poly_vec_k_inv_ntt_to_mont_volatile %d\n", counter);
}

static void poly_vec_k_add_volatile(volatile PolyVecK *w, volatile PolyVecK *u, PolyVecK *v) {
	uint32_t counter = 0;
	// int32_t temp = 0;
    for(int i = 0; i < K; ++i) {
		memmove(&ubuf.temp_poly, &w->vec[i], sizeof(Poly));
		for(int j = 0; j < N; ++j) {
			ubuf.temp_poly.coeffs[j] = ubuf.temp_poly.coeffs[j] + v->vec[i].coeffs[j];
		}
		nvm_write((void*)&w->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_k_add_volatile %d\n", counter);
}

static void poly_vec_k_ntt_volatile(PolyVecK *v) {
	uint32_t counter = 0;
    for(int i = 0; i < K; i++) {
		memmove(&ubuf.temp_poly, &v->vec[i], sizeof(Poly));
		uint32_t count = 0, start = 0, j = 0, k = 0;
		int32_t zeta = 0, t = 0;

		k = 0;
		int32_t temp = 0;
		for(count = 128; count > 0; count >>= 1) {
			for(start = 0; start < N; start = j + count) {
				k++;
				zeta = ZETAS[k];
				for(j = start; j < start+count; j++) {
					int32_t t2 = 0;
					int64_t a_reduce = (int64_t)zeta * (int64_t)ubuf.temp_poly.coeffs[j+count];
					t2 = (int32_t) ((int64_t)(int32_t)a_reduce * Q_INV);
					t2 = (int32_t) ((a_reduce - (int64_t)t2*Q_CONST) >> 32);
					t = t2;
					// t = montgomery_reduce((int64_t)zeta * (int64_t)(*a)[j+count]);
					ubuf.temp_poly.coeffs[j+count] = ubuf.temp_poly.coeffs[j] - t;
					// nvm_write((void *)&v->vec[i].coeffs[j+count], &temp, sizeof(int32_t));counter++;
					// (*a)[j+count] = v->vec[i].[j] - t;
					ubuf.temp_poly.coeffs[j] = ubuf.temp_poly.coeffs[j] + t;
					// nvm_write((void *)&v->vec[i].coeffs[j], &temp, sizeof(int32_t));counter++;
					// (*a)[j] = v->vec[i].[j] + t;
				}
			}
		}
		nvm_write((void*)&v->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_k_ntt_volatile %u\n", counter);
}

static void poly_vec_k_c_addq_volatile(volatile PolyVecK *v) {
	uint32_t counter = 0;
    for(int i = 0; i < K; ++i) {
		memmove(&ubuf.temp_poly, &v->vec[i], sizeof(Poly));
		for(int j = 0; j < N; ++j) {
			int32_t a = ubuf.temp_poly.coeffs[j];
			a += (a >> 31) & Q_CONST;
			ubuf.temp_poly.coeffs[j] = a;
		}
		nvm_write((void*)&v->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_k_c_addq_volatile %d\n", counter);
}

static void poly_vec_k_power2_round_volatile(volatile PolyVecK *v1, volatile PolyVecK *v0, volatile PolyVecK *v) {
    uint32_t counter = 0;
	int32_t temp = 0;
	for(int i = 0; i < K; ++i) {
		memmove(&ubuf.temp_poly, &v1->vec[i], sizeof(Poly));
		for(int j = 0; j < N; ++j) {
			int32_t a1 = 0;

			a1 = (v->vec[i].coeffs[j] + (1 << (D - 1)) - 1) >> D;
			temp = v->vec[i].coeffs[j] - (a1 << D);
			nvm_write((void*)&v0->vec[i].coeffs[j], &temp, sizeof(int32_t));counter++;
			// v0->vec[i].coeffs[j] = v->vec[i].coeffs[j] - (a1 << D);
			ubuf.temp_poly.coeffs[j] = a1;
		}
		nvm_write((void*)&v1->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_k_power2_round_volatile %d\n", counter);
}

static void poly_vec_k_power2_round_volatile_keypair(volatile PolyVecK *v1, volatile PolyVecK *v) {
    uint32_t counter = 0;
	for(int i = 0; i < K; ++i) {
		memmove(&ubuf.temp_poly, &v1->vec[i], sizeof(Poly));
		for(int j = 0; j < N; ++j) {
			int32_t a1 = 0;

			a1 = (v->vec[i].coeffs[j] + (1 << (D - 1)) - 1) >> D;
			// v0->vec[i].coeffs[j] = v->vec[i].coeffs[j] - (a1 << D);
			ubuf.temp_poly.coeffs[j] = a1;
		}
		nvm_write((void*)&v1->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_k_power2_round_volatile %d\n", counter);
}

static void pack_pk_volatile(volatile uint8_t (*pkb)[CRYPTO_PUBLIC_KEY_BYTES], uint8_t rho[SEED_BYTES], volatile PolyVecK *t1) {
	uint32_t counter = 0;
	uint8_t temp1[SEED_BYTES] = {0};
	for(int i = 0; i < SEED_BYTES; ++i) {
		temp1[i] = rho[i];
	}
	nvm_write((void*)&N_storage.pk[0], (void *)&temp1[0], sizeof(uint8_t)*SEED_BYTES);counter++;
	uint8_t tmp = 0;
	uint8_t temp2[POLY_T1_PACKED_BYTES] = {0};
	for(int b = 0; b < K; b++) {
		explicit_bzero(&temp2, sizeof(temp2));
		for(int i = 0; i < N/4; i++) {
			temp2[5*i+0] = (uint8_t)(t1->vec[b].coeffs[4*i+0] >> 0);

			temp2[5*i+1] = (uint8_t)((t1->vec[b].coeffs[4*i+0] >> 8) | (t1->vec[b].coeffs[4*i+1] << 2));

			temp2[5*i+2] = (uint8_t)((t1->vec[b].coeffs[4*i+1] >> 6) | (t1->vec[b].coeffs[4*i+2] << 4));

			temp2[5*i+3] = (uint8_t)((t1->vec[b].coeffs[4*i+2] >> 4) | (t1->vec[b].coeffs[4*i+3] << 6));

			temp2[5*i+4] = (uint8_t)((t1->vec[b].coeffs[4*i+3] >> 2));
		}
		nvm_write((void*)&N_storage.pk[SEED_BYTES + b*POLY_T1_PACKED_BYTES], (void *)&temp2[0], sizeof(uint8_t)*POLY_T1_PACKED_BYTES);counter++;
	}
	PRINTF("pack_pk_volatile %d\n", counter);
}

ErrorCode crypto_sign_keypair(uint8_t (*seed)[SEED_BYTES]) {
	uint8_t tr[TR_BYTES] = {0};
	uint8_t rho[SEED_BYTES] = {0}; 
	uint8_t key[SEED_BYTES] = {0}; 
	uint8_t rhoPrime[CRH_BYTES] = {0};

	// union_s1hat_t0 u;
	
	PolyVecL s1; explicit_bzero(&s1, sizeof(s1));
	PolyVecK s2; explicit_bzero(&s2, sizeof(s2));

	if(seed == NULL) {
		uint8_t seed_new[SEED_BYTES] = {0};
		cx_rng(seed_new, SEED_BYTES);
		seed = &seed_new;		
	}
	/* Expand 32 bytes of randomness into rho, rhoprime and key */
	shake256_ctx ctx;
	shake256_init(&ctx);
	shake256_absorb(&ctx, seed, SEED_BYTES);
	uint8_t extraData[2] = {K, L};
	shake256_absorb(&ctx, extraData, 2);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, rho, SEED_BYTES);
	shake256_squeeze(&ctx, rhoPrime, CRH_BYTES);
	shake256_squeeze(&ctx, key, SEED_BYTES);
	shake256_clear(&ctx);

	int err = 0;
	/* Sample short vectors s1 and s2 */
	err = poly_vec_l_uniform_eta(&s1, &rhoPrime, 0); 
	if(err != 0) {
		return err;
	}
	err = poly_vec_k_uniform_eta(&s2, &rhoPrime, L); 
	if(err != 0) {
		return err;
	}
	/* Matrix-vector multiplication */
	// u.s1hat = s1;
	poly_vec_l_ntt(&s1);
	combined_method(&N_storage.t1, &s1, rho);
	poly_vec_k_reduce_volatile(&N_storage.t1);
	poly_vec_k_inv_ntt_to_mont_volatile(&N_storage.t1);

	// /* Add noise vector s2 */
	poly_vec_k_add_volatile(&N_storage.t1, &N_storage.t1, &s2); //Error here
	
	// /* Extract t1 and write public key */
	poly_vec_k_c_addq_volatile(&N_storage.t1);
	poly_vec_k_power2_round_volatile_keypair(&N_storage.t1, &N_storage.t1);	
	pack_pk_volatile(&N_storage.pk, rho, &N_storage.t1);

	return 0;
}

static void poly_vec_l_uniform_gamma1_volatile(volatile PolyVecL *v, uint8_t seed[CRH_BYTES], uint16_t nonce) {
    uint32_t counter = 0;
	// static uint8_t buf[POLY_UNIFORM_GAMMA1_N_BLOCKS * STREAM_256_BLOCK_BYTES] = {0};
	for(int j = (uint16_t)0; j < L; j++) {
		memmove(&ubuf.temp_poly, &v->vec[j], sizeof(Poly));
		// explicit_bzero(buf, POLY_UNIFORM_GAMMA1_N_BLOCKS * STREAM_256_BLOCK_BYTES);

		shake256_ctx ctx;
		shake256_init(&ctx);
		shake256_absorb(&ctx, seed, CRH_BYTES);
		uint16_t nonce_val = L*nonce+j;
		uint8_t nonces[2] = {(uint8_t)nonce_val, (uint8_t)(nonce_val >> 8)};
		shake256_absorb(&ctx, nonces, 2);
		shake256_finalize(&ctx);
		shake256_squeeze_volatile(&ctx, N_storage.buf, POLY_UNIFORM_GAMMA1_N_BLOCKS * STREAM_256_BLOCK_BYTES);
		shake256_clear(&ctx);
		int32_t temp = 0;
		
		for(int i = 0; i < N/2; i++) {
			ubuf.temp_poly.coeffs[2*i+0] = (int32_t)(N_storage.buf[5*i+0]);

			ubuf.temp_poly.coeffs[2*i+0] = ubuf.temp_poly.coeffs[2*i+0] | (int32_t)((uint32_t)(N_storage.buf[5*i+1]) << 8);

			ubuf.temp_poly.coeffs[2*i+0] = ubuf.temp_poly.coeffs[2*i+0] | (int32_t)((uint32_t)(N_storage.buf[5*i+2]) << 16);

			ubuf.temp_poly.coeffs[2*i+0] = ubuf.temp_poly.coeffs[2*i+0] & 0xFFFFF;		

			ubuf.temp_poly.coeffs[2*i+1] = (int32_t)(N_storage.buf[5*i+2] >> 4);

			ubuf.temp_poly.coeffs[2*i+1] = ubuf.temp_poly.coeffs[2*i+1] | (int32_t)((uint32_t)(N_storage.buf[5*i+3]) << 4);

			ubuf.temp_poly.coeffs[2*i+1] = ubuf.temp_poly.coeffs[2*i+1] | (int32_t)((uint32_t)(N_storage.buf[5*i+4]) << 12);

			ubuf.temp_poly.coeffs[2*i+0] = ubuf.temp_poly.coeffs[2*i+0] & 0xFFFFF; 

			ubuf.temp_poly.coeffs[2*i+0] = GAMMA1 - ubuf.temp_poly.coeffs[2*i+0];

			ubuf.temp_poly.coeffs[2*i+1] = GAMMA1 - ubuf.temp_poly.coeffs[2*i+1];
		}
		nvm_write((void*)&v->vec[j], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_l_uniform_gamma1_volatile %u\n", counter);
}

ErrorCode crypto_sign_signature_internal(uint8_t *sig, size_t sig_len, uint8_t *m, size_t m_len, uint8_t *pre, size_t pre_len, uint8_t rnd[RND_BYTES], uint8_t (*sk)[CRYPTO_SECRET_KEY_BYTES]) {
	uint8_t rho[SEED_BYTES] = {0};
	uint8_t key[SEED_BYTES] = {0};
	uint8_t tr[TR_BYTES] = {0};
	uint8_t mu[CRH_BYTES] = {0};
	uint8_t rhoPrime[CRH_BYTES] = {0};

	PolyVecL s1, y, z;
	explicit_bzero(&s1, sizeof(s1));
	explicit_bzero(&y, sizeof(y));
	explicit_bzero(&z, sizeof(z));

	PolyVecL mat[K];
	explicit_bzero(mat, sizeof(mat));

	PolyVecK s2, t0, w1, h, w0;
	explicit_bzero(&s2, sizeof(s2));
	explicit_bzero(&t0, sizeof(t0));
	explicit_bzero(&w1, sizeof(w1));
	explicit_bzero(&h, sizeof(h));
	explicit_bzero(&w0, sizeof(w0));
	
	Poly cp;
	explicit_bzero(&cp, sizeof(cp));

	uint16_t nonce = 0;

	unpack_sk(&rho, &tr, &key, &t0, &s1, &s2, sk);

	/* Compute mu = CRH(tr, 0, ctxlen, ctx, msg) */
	shake256_ctx ctx;
	shake256_init(&ctx);
	shake256_absorb(&ctx, tr, TR_BYTES);
	shake256_absorb(&ctx, pre, pre_len);
	shake256_absorb(&ctx, m, m_len);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, mu, CRH_BYTES);
	shake256_clear(&ctx);

	/* Compute rhoprime = CRH(key, rnd, mu) */
	shake256_init(&ctx);
	shake256_absorb(&ctx, key, SEED_BYTES);
	shake256_absorb(&ctx, rnd, RND_BYTES);
	shake256_absorb(&ctx, mu, CRH_BYTES);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, rhoPrime, CRH_BYTES);
	shake256_clear(&ctx);

	/* Expand matrix and transform vectors */
	int err = poly_vec_matrix_expand(&mat, &rho);
	if(err != 0) {
		return err;
	}
	poly_vec_l_ntt(&s1);
	poly_vec_k_ntt(&s2);
	poly_vec_k_ntt(&t0);

rej:

	/* Sample intermediate vector y */
	poly_vec_l_uniform_gamma1(&y, rhoPrime, nonce);
	nonce++;

	/* Matrix-vector multiplication */
	z = y;
	poly_vec_l_ntt(&z);
	poly_vec_matrix_pointwise_montgomery(&w1, &mat, &z);
	poly_vec_k_reduce(&w1);
	poly_vec_k_inv_ntt_to_mont(&w1);

	/* Decompose w and call the random oracle */
	poly_vec_k_c_addq(&w1);
	poly_vec_k_decompose(&w1, &w0, &w1);
	err = poly_vec_k_pack_w1(sig, sig_len, &w1);
	if(err != 0) {
		return err;
	}

	shake256_init(&ctx);
	shake256_absorb(&ctx, mu, CRH_BYTES);
	shake256_absorb(&ctx, sig, K*POLY_W1_PACKED_BYTES);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, sig, CTILDE_BYTES);
	shake256_clear(&ctx);
	err = poly_challenge(&cp, sig, sig_len);
	if(err != 0) {
		return err;
	}
	poly_ntt(&cp);

	/* Compute z, reject if it reveals secret */
	poly_vec_l_pointwise_poly_montgomery(&z, &cp, &s1);
	poly_vec_l_inv_ntt_to_mont(&z);
	poly_vec_l_add(&z, &z, &y);
	poly_vec_l_reduce(&z);
	if(poly_vec_l_chk_norm(&z, GAMMA1-BETA) != 0) {
		goto rej;
	}

	/* Check that subtracting cs2 does not change high bits of w and low bits
	 * do not reveal secret information */
	poly_vec_k_pointwise_poly_montgomery(&h, &cp, &s2);
	poly_vec_k_inv_ntt_to_mont(&h);
	poly_vec_k_sub(&w0, &w0, &h);
	poly_vec_k_reduce(&w0);
	if(poly_vec_k_chk_norm(&w0, GAMMA2-BETA) != 0) {
		goto rej;
	}

	/* Compute hints for w1 */
	poly_vec_k_pointwise_poly_montgomery(&h, &cp, &t0);
	poly_vec_k_inv_ntt_to_mont(&h);
	poly_vec_k_reduce(&h);
	if(poly_vec_k_chk_norm(&h, GAMMA2) != 0) {
		goto rej;
	}

	poly_vec_k_add(&w0, &w0, &h);
	uint32_t n = poly_vec_k_make_hint(&h, &w0, &w1);
	if(n > OMEGA) {
		goto rej;
	}
	uint8_t *c = rhoPrime + CRH_BYTES;
	copy_array(c, CTILDE_BYTES, sig, CTILDE_BYTES);
	err = pack_sig(sig, sig_len, c, &z, &h); 
	if(err != 0) {
		return err;
	}
	return 0;
}

static void poly_vec_l_ntt_volatile(volatile PolyVecL *v) {
	uint32_t counter = 0;
    for(int i = 0; i < L; i++) {
		memmove(&ubuf.temp_poly, &v->vec[i], sizeof(Poly));
		uint32_t count = 0, start = 0, j = 0, k = 0;
		int32_t zeta = 0, t = 0;

		k = 0;
		int32_t temp = 0;
		for(count = 128; count > 0; count >>= 1) {
			for(start = 0; start < N; start = j + count) {
				k++;
				zeta = ZETAS[k];
				for(j = start; j < start+count; j++) {
					int32_t t1 = 0;
					int64_t a_reduce = (int64_t)zeta * (int64_t)ubuf.temp_poly.coeffs[j+count];
					t1 = (int32_t) ((int64_t)(int32_t)a_reduce * Q_INV);
					t1 = (int32_t) ((a_reduce - (int64_t)t1*Q_CONST) >> 32);
					t = t1;
					ubuf.temp_poly.coeffs[j+count] = ubuf.temp_poly.coeffs[j] - t;
					ubuf.temp_poly.coeffs[j] = ubuf.temp_poly.coeffs[j] + t;
				}
			}
		}
		nvm_write((void*)&v->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_l_ntt_volatile %u\n", counter);
}

static void poly_vec_k_decompose_volatile(volatile PolyVecK *v1, volatile PolyVecK *v0, volatile PolyVecK *v) {
	uint32_t counter = 0;
	int32_t temp = 0;
	// static Poly ubuf.temp_poly_2;
	for(int i = 0; i < K; i++) {
		memmove(&ubuf.temp_poly, &v1->vec[i], sizeof(Poly));
		// memmove(&ubuf.temp_poly_2, &v0->vec[i], sizeof(Poly));
		for(int j = 0; j < N; j++) {
			int32_t a1_result = (v->vec[i].coeffs[j] + 127) >> 7;
			a1_result = (a1_result*1025 + (1 << 21)) >> 22;
			a1_result &= 15;

			temp = v->vec[i].coeffs[j] - a1_result*2*GAMMA2;
			// ubuf.temp_poly_2.coeffs[j] = temp;
			nvm_write((void *)&v0->vec[i].coeffs[j], &temp, sizeof(int32_t));counter++;
			temp = v0->vec[i].coeffs[j] - ((((Q_CONST-1)/2 - v0->vec[i].coeffs[j]) >> 31) & Q_CONST);
			// ubuf.temp_poly_2.coeffs[j] = temp;
			nvm_write((void *)&v0->vec[i].coeffs[j], &temp, sizeof(int32_t));counter++;
			ubuf.temp_poly.coeffs[j] = a1_result;
			// nvm_write((void *)&v1->vec[i].coeffs[j], &a1_result, sizeof(int32_t));counter++;
		}
		nvm_write((void*)&v1->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
		// nvm_write((void*)&v0->vec[i], &ubuf.temp_poly_2, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_k_decompose_volatile %u\n", counter);
}

static void poly_vec_k_pack_w1_volatile(volatile uint8_t *r, volatile PolyVecK *w1) {
	uint32_t counter = 0;
	int32_t temp = 0;
	uint8_t temp_buffer[POLY_W1_PACKED_BYTES] = {0};
	for(int i = 0; i < K; i++) {
		explicit_bzero(&temp_buffer, sizeof(temp_buffer));
		for(int j = 0; j < N/2; j++) {
			temp = (uint8_t)(w1->vec[i].coeffs[2*j+0] | (w1->vec[i].coeffs[2*j+1] << 4));
			temp_buffer[j] = temp;
			// nvm_write((void *)&r[j + i*POLY_W1_PACKED_BYTES], &temp, sizeof(uint8_t));counter++;
		}
		nvm_write((void *)&r[i*POLY_W1_PACKED_BYTES], &temp_buffer[0], sizeof(uint8_t)*POLY_W1_PACKED_BYTES);counter++;
	}
	PRINTF("poly_vec_k_pack_w1_volatile %u\n", counter);
}

static void poly_vec_k_pack_w1_volatile_verify(volatile uint8_t r[K*POLY_W1_PACKED_BYTES], volatile PolyVecK *w1) {
	uint8_t temp_buffer[POLY_W1_PACKED_BYTES] = {0};
	uint32_t counter = 0;
	for(int i = 0; i < K; i++) {
		explicit_bzero(&temp_buffer, sizeof(temp_buffer));
		for(int j = 0; j < N/2; j++) {
			temp_buffer[j] = (uint8_t)(w1->vec[i].coeffs[2*j+0] | (w1->vec[i].coeffs[2*j+1] << 4));
			// r[j + i*POLY_W1_PACKED_BYTES] = (uint8_t)(w1->vec[i].coeffs[2*j+0] | (w1->vec[i].coeffs[2*j+1] << 4));
		}
		nvm_write((void *)&r[i*POLY_W1_PACKED_BYTES], (void *)&temp_buffer[0], sizeof(uint8_t)*POLY_W1_PACKED_BYTES);counter++;
	}
	PRINTF("poly_vec_k_pack_w1_volatile_verify %u\n", counter);
}

static void poly_vec_l_pointwise_poly_montgomery_volatile(volatile PolyVecL *r, volatile Poly *a, PolyVecL *v) {
    uint32_t counter = 0;
	for(int i = 0; i < L; i++) {
		memmove(&ubuf.temp_poly, &r->vec[i], sizeof(Poly));
		for(int j = 0; j < N; j++) {
			int32_t t = 0;
			int64_t a_reduce = (int64_t)(a->coeffs[j]) * (int64_t)(v->vec[i].coeffs[j]);
			t = (int32_t) ((int64_t)(int32_t)a_reduce * Q_INV);
			t = (int32_t) ((a_reduce - (int64_t)t*Q_CONST) >> 32);
			ubuf.temp_poly.coeffs[j] = t;
		}
		nvm_write((void*)&r->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_l_pointwise_poly_montgomery_volatile %u\n", counter);
}

static void poly_vec_l_inv_ntt_to_mont_volatile(volatile PolyVecL *v) {
	uint32_t counter = 0;
    for(int i = 0; i < L; i++) {
		memmove(&ubuf.temp_poly, &v->vec[i], sizeof(Poly));
		uint32_t count = 0, start = 0, j = 0, k = 0;
		int32_t zeta = 0, t = 0;
		int32_t f = (int32_t)41978;

		k = 256;
		int32_t temp = 0;
		for(count = 1; count < N; count <<= 1) {
			for(start = 0; start < N; start = j + count) {
				k--;
				zeta = -ZETAS[k];
				for(j = start; j < start+count; j++) {
					t = ubuf.temp_poly.coeffs[j];
					temp = t + ubuf.temp_poly.coeffs[j+count];
					ubuf.temp_poly.coeffs[j] = temp;
					// nvm_write((void *)&v->vec[i].coeffs[j], &temp, sizeof(int32_t));counter++;
					temp = t - ubuf.temp_poly.coeffs[j+count];
					ubuf.temp_poly.coeffs[j+count] = temp;
					// nvm_write((void *)&v->vec[i].coeffs[j+count], &temp, sizeof(int32_t));counter++;
					int32_t t2 = 0;
					int64_t a = (int64_t)zeta * (int64_t)ubuf.temp_poly.coeffs[j+count];
					t2 = (int32_t) ((int64_t)(int32_t)a * Q_INV);
					t2 = (int32_t) ((a - (int64_t)t2*Q_CONST) >> 32);
					ubuf.temp_poly.coeffs[j+count] = t2;
					// nvm_write((void *)&v->vec[i].coeffs[j+count], &t2, sizeof(int32_t));counter++;
				}
			}
		}

		for(j = 0; j < N; j++) {
			int32_t t3 = 0;
			int64_t a = (int64_t)f * (int64_t)ubuf.temp_poly.coeffs[j];
			t3 = (int32_t) ((int64_t)(int32_t)a * Q_INV);
			t3 = (int32_t) ((a - (int64_t)t3*Q_CONST) >> 32);
			ubuf.temp_poly.coeffs[j] = t3;
			// nvm_write((void *)&v->vec[i].coeffs[j], &t3, sizeof(int32_t));counter++;
		}
		nvm_write((void*)&v->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_l_inv_ntt_to_mont_volatile %u\n", counter);
}

static void poly_vec_l_add_volatile(volatile PolyVecL *w, volatile PolyVecL *u, volatile PolyVecL *v) {
	// int32_t temp = 0;
	uint32_t counter = 0;
    for(int i = 0; i < L; i++) {
		memmove(&ubuf.temp_poly, &w->vec[i], sizeof(Poly));
		for(int j = 0; j < N; j++) {
			ubuf.temp_poly.coeffs[j] = ubuf.temp_poly.coeffs[j] + v->vec[i].coeffs[j];
		}
		nvm_write((void*)&w->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_l_add_volatile %u\n", counter);
}

static void poly_vec_l_reduce_volatile(volatile PolyVecL *v) {
	uint32_t counter = 0;
    for(int i = 0; i < L; i++) {
		memmove(&ubuf.temp_poly, &v->vec[i], sizeof(Poly));
		for(int j = 0; j < N; j++) {
			int32_t t = 0;

			t = (v->vec[i].coeffs[j] + (1 << 22)) >> 23;
			t = v->vec[i].coeffs[j] - t*Q_CONST;
			ubuf.temp_poly.coeffs[j] = t;
		}
		nvm_write((void*)&v->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_l_reduce_volatile %u\n", counter);
}

static void poly_vec_k_sub_volatile(volatile PolyVecK *w, volatile PolyVecK *u, PolyVecK *v) {
	// int32_t temp = 0;
	uint32_t counter = 0;
    for(int i = 0; i < K; i++) {
		memmove(&ubuf.temp_poly, &w->vec[i], sizeof(Poly));
		for(int j = 0; j < N; j++) {
			ubuf.temp_poly.coeffs[j] = u->vec[i].coeffs[j] - v->vec[i].coeffs[j]; 
		}
		nvm_write((void*)&w->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_k_sub_volatile %u\n", counter);
}

static ErrorCode pack_sig_volatile(volatile uint8_t sig[], size_t sigb_len, uint8_t c[CTILDE_BYTES], volatile PolyVecL *z, PolyVecK *h) {
    uint32_t counter = 0;
	if(sigb_len != CRYPTO_BYTES) {
		return ERR_INVALID_SIGB_LENGTH;
	}
	uint8_t temp1[CTILDE_BYTES] = {0};
	for(int i = 0; i < CTILDE_BYTES; i++) {
		temp1[i] = c[i]; 
	}
	nvm_write((void *)&sig[0], (void *)&temp1[0], sizeof(uint8_t)*CTILDE_BYTES);counter++;
	
	// uint8_t temp2[POLY_Z_PACKED_BYTES] = {0};
	// int32_t temp = 0;
	uint8_t temp = 0;
	for(int b = 0; b < L; b++) {
		// explicit_bzero(&temp2, sizeof(temp2));
		uint32_t t[4] = {0};

		for(int i = 0; i < N/2; i++) {
			t[0] = (uint32_t)(GAMMA1 - z->vec[b].coeffs[2*i+0]);
			t[1] = (uint32_t)(GAMMA1 - z->vec[b].coeffs[2*i+1]);

			// temp2[5*i+0] = (uint8_t)(t[0]);
			temp = (uint8_t)(t[0]);
			nvm_write((void *)&sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES + 5*i+0], &temp, sizeof(uint8_t));counter++;

			// temp2[5*i+1] = (uint8_t)(t[0] >> 8);
			temp = (uint8_t)(t[0] >> 8);
			nvm_write((void *)&sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES + 5*i+1], &temp, sizeof(uint8_t));counter++;

			// temp2[5*i+2] = (uint8_t)(t[0] >> 16);
			temp = (uint8_t)(t[0] >> 16);
			nvm_write((void *)&sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES + 5*i+2], &temp, sizeof(uint8_t));counter++;

			// temp2[5*i+2] = temp2[5*i+2] | (uint8_t)(t[1] << 4);
			temp = sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES + 5*i+2] | (uint8_t)(t[1] << 4);
			nvm_write((void *)&sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES + 5*i+2], &temp, sizeof(uint8_t));counter++;
			
			// temp2[5*i+3] = (uint8_t)(t[1] >> 4);
			temp = (uint8_t)(t[1] >> 4);
			nvm_write((void *)&sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES + 5*i+3], &temp, sizeof(uint8_t));counter++;

			// temp2[5*i+4] = (uint8_t)(t[1] >> 12);
			temp = (uint8_t)(t[1] >> 12);
			nvm_write((void *)&sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES + 5*i+4], &temp, sizeof(uint8_t));counter++;
		}
		// nvm_write((void *)&sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES], (void *)&temp2[0], sizeof(uint8_t)*POLY_Z_PACKED_BYTES);counter++;
	}

	/* Encode h */
	// temp = 0;
	uint8_t temp3[OMEGA+K] = {0};
	for(int i = 0; i < OMEGA+K; i++) {
		temp3[i] = 0;
	}
	nvm_write((void *)&sig[CTILDE_BYTES + L*POLY_Z_PACKED_BYTES], (void *)&temp3[0], sizeof(uint8_t)*(OMEGA+K));counter++;

	explicit_bzero(&temp3, sizeof(temp3));
	int32_t k = 0;
	for(int i = 0; i < K; i++) {
		for(int j = 0; j < N; j++) {
			if(h->vec[i].coeffs[j] != 0) {
				temp3[k] = (uint8_t)j;
				k++;
			}
			temp3[OMEGA+i] = (uint8_t)k;
		}
	}
	nvm_write((void *)&sig[CTILDE_BYTES + L*POLY_Z_PACKED_BYTES], (void *)&temp3[0], sizeof(uint8_t)*(OMEGA+K));counter++;
	PRINTF("pack_sig_volatile %u\n", counter);
	return ERR_NONE;
}

static void poly_vec_k_pointwise_poly_montgomery_volatile(volatile PolyVecK *r, Poly *a, PolyVecK *v) {
	uint32_t counter = 0;
	for(int i = 0; i < K; i++) {
		memmove(&ubuf.temp_poly, &r->vec[i], sizeof(Poly));
		for(int j = 0; j < N; j++) {
			int32_t t = 0;
			int64_t a_reduce = (int64_t)(a->coeffs[j]) * (int64_t)(v->vec[i].coeffs[j]);
			t = (int32_t) ((int64_t)(int32_t)a_reduce * Q_INV);
			t = (int32_t) ((a_reduce - (int64_t)t*Q_CONST) >> 32);
			ubuf.temp_poly.coeffs[j] = t;
		}
		nvm_write((void*)&r->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_k_pointwise_poly_montgomery_volatile %u\n", counter);
}

static ErrorCode poly_challenge_volatile(volatile Poly *c, volatile uint8_t seed[], size_t seed_len) {
	uint32_t counter = 0;
	uint32_t pos = 0, b = 0;
	uint8_t buf[SHAKE256_RATE];
	shake256_ctx ctx;
	shake256_init(&ctx);
	shake256_absorb(&ctx, seed, seed_len);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, buf, SHAKE256_RATE);

	uint64_t signs = 0;
	for(uint64_t i = 0; i < 8; i++) {
		signs |= (uint64_t)buf[i] << (8 * i);
	}
	pos = 8;

	int32_t temp = 0;
	for(int i = 0; i < N; i++) {
		nvm_write((void *)&c->coeffs[i], &temp, sizeof(int32_t));counter++;
	}
	for(int i = N - TAU; i < N; i++) {
		while(1) {
			if(pos >= SHAKE256_RATE) {
				shake256_squeeze(&ctx, buf, SHAKE256_RATE);
				pos = 0;
			}

			b = (uint32_t)buf[pos];
			pos++;
			if(!(b > (uint32_t)i)) {
				break;
			}
		}

		temp = c->coeffs[b];
		nvm_write((void *)&c->coeffs[i], &temp, sizeof(int32_t));counter++;
		temp = (int32_t)(1 - 2*(signs&1));
		nvm_write((void *)&c->coeffs[b], &temp, sizeof(int32_t));counter++;
		signs >>= 1;
	}
	shake256_clear(&ctx);
	PRINTF("poly_challenge_volatile %u\n", counter);
	return 0;
}

static void poly_ntt_volatile(Poly *a) {
	memmove(&ubuf.temp_poly, a, sizeof(Poly));
	uint32_t counter = 0;
    uint32_t count = 0, start = 0, j = 0, k = 0;
	int32_t zeta = 0, t = 0;

	k = 0;
	int32_t temp = 0;
	for(count = 128; count > 0; count >>= 1) {
		for(start = 0; start < N; start = j + count) {
			k++;
			zeta = ZETAS[k];
			for(j = start; j < start+count; j++) {
				int32_t t2 = 0;
				int64_t a_reduce = (int64_t)zeta * (int64_t)ubuf.temp_poly.coeffs[j+count];
				t2 = (int32_t) ((int64_t)(int32_t)a_reduce * Q_INV);
				t2 = (int32_t) ((a_reduce - (int64_t)t2*Q_CONST) >> 32);
				t = t2;
				temp = ubuf.temp_poly.coeffs[j] - t;
				ubuf.temp_poly.coeffs[j+count] = temp;
				// nvm_write((void *)&a->coeffs[j+count], &temp, sizeof(int32_t));counter++;
				temp = ubuf.temp_poly.coeffs[j] + t;
				ubuf.temp_poly.coeffs[j] = temp;
				// nvm_write((void *)&a->coeffs[j], &temp, sizeof(int32_t));counter++;
			}
		}
	}
	nvm_write(a, &ubuf.temp_poly, sizeof(Poly));counter++;
	PRINTF("poly_ntt_volatile %u\n", counter);
}

static uint32_t poly_vec_k_make_hint_volatile(volatile PolyVecK *h, volatile PolyVecK *v0, volatile PolyVecK *v1) {
    uint32_t counter = 0;
	uint32_t s = 0;
    for(int i = 0; i < K; i++) {
		memmove(&ubuf.temp_poly, &h->vec[i], sizeof(Poly));
		uint32_t s2 = 0;
		for(int j = 0; j < N; j++) {
			int32_t temp = 0;
			if(v0->vec[i].coeffs[j] > GAMMA2 || v0->vec[i].coeffs[j] < -GAMMA2 || (v0->vec[i].coeffs[j] == -GAMMA2 && v1->vec[i].coeffs[j] != 0)) {
				temp = 1;
			} else {
				temp = 0;
			}

			ubuf.temp_poly.coeffs[j] = temp;
			// nvm_write((void *)&h->vec[i].coeffs[j], &temp, sizeof(int32_t));counter++;
			s2 += (uint32_t)(ubuf.temp_poly.coeffs[j]);
		}
		nvm_write((void*)&h->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
		s += s2;
	}
	PRINTF("poly_vec_k_make_hint_volatile %u\n", counter);
	return s;
}

typedef union {
	PolyVecK s2;
	PolyVecL s1hat;
} u_s2_s1hat;

ErrorCode crypto_sign_optimized(const uint32_t bip32_path[], size_t bip32_path_len, uint8_t *message, size_t message_len) {
	//Data validation
	if (message == NULL || message_len == 0) {
        return ERR_INVALID_MESSAGE;
    }
	if(CTX_LEN > 255) {
		return ERR_INVALID_CONTEXT_LENGTH;
	}

	//Message sign prepare
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
    uint8_t seed[32] = {0};
    for(int i = 0; i < 32; i++) {
        seed[i] = raw_seed[i];
    }

	// PRINTF("MLDSA87 SEED: ");
    // for(int i = 0; i < SEED_BYTES; i++) {
    //     PRINTF("%02x", seed[i]);
    // }
    // PRINTF("\n");
	
	uint8_t rnd[RND_BYTES] = {0};
	uint8_t pre[CTX_LEN + 2] = {0};
	pre[0] = 0;
	pre[1] = (uint8_t)CTX_LEN;
	for(int i = 0; i < CTX_LEN; ++i) {
		pre[i+2] = CTX[i];
	}

	//From keypair method
	uint8_t tr[TR_BYTES] = {0};
	uint8_t rho[SEED_BYTES] = {0}; 
	uint8_t key[SEED_BYTES] = {0}; 
	uint8_t rhoPrime[CRH_BYTES] = {0};

	u_s2_s1hat u;
	PolyVecL s1;
	// PolyVecK s2;

	/* Expand 32 bytes of randomness into rho, rhoprime and key */
	shake256_ctx ctx;
	shake256_init(&ctx);
	shake256_absorb(&ctx, seed, SEED_BYTES);
	uint8_t extraData[2] = {K, L};
	shake256_absorb(&ctx, extraData, 2);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, rho, SEED_BYTES);
	shake256_squeeze(&ctx, rhoPrime, CRH_BYTES);
	shake256_squeeze(&ctx, key, SEED_BYTES);
	shake256_clear(&ctx);

	/* Sample short vectors s1 and s2 */
	err = poly_vec_l_uniform_eta(&s1, &rhoPrime, 0); 
	if(err != 0) {
		return err;
	}
	/* Matrix-vector multiplication */
	u.s1hat = s1;
	poly_vec_l_ntt(&u.s1hat);
	combined_method(&N_storage.t1, &u.s1hat, rho); //TODO: fix this method
	poly_vec_k_reduce_volatile(&N_storage.t1);
	poly_vec_k_inv_ntt_to_mont_volatile(&N_storage.t1);

	/* Add noise vector s2 */
	err = poly_vec_k_uniform_eta(&u.s2, &rhoPrime, L); 
	if(err != 0) {
		return err;
	}
	poly_vec_k_add_volatile(&N_storage.t1, &N_storage.t1, &u.s2);
	
	/* Extract t1 and write public key */
	poly_vec_k_c_addq_volatile(&N_storage.t1);
	poly_vec_k_power2_round_volatile(&N_storage.t1, &N_storage.pack1.t0, &N_storage.t1);
	pack_pk_volatile(&N_storage.pk, &rho, &N_storage.t1);

	shake256_init(&ctx);
	shake256_absorb(&ctx, &N_storage.pk, CRYPTO_PUBLIC_KEY_BYTES);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, tr, TR_BYTES);
	shake256_clear(&ctx);

	//From signing method
	uint8_t mu[CRH_BYTES] = {0};
	explicit_bzero(rhoPrime, CRH_BYTES);

	uint16_t nonce = 0;

	/* Compute mu = CRH(tr, 0, ctxlen, ctx, msg) */
	shake256_init(&ctx);
	shake256_absorb(&ctx, tr, TR_BYTES);
	shake256_absorb(&ctx, pre, CTX_LEN + 2);
	shake256_absorb(&ctx, message, message_len);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, mu, CRH_BYTES);
	shake256_clear(&ctx);

	/* Compute rhoprime = CRH(key, rnd, mu) */
	shake256_init(&ctx);
	shake256_absorb(&ctx, key, SEED_BYTES);
	shake256_absorb(&ctx, rnd, RND_BYTES);
	shake256_absorb(&ctx, mu, CRH_BYTES);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, rhoPrime, CRH_BYTES);
	shake256_clear(&ctx);

// 	/* Expand matrix and transform vectors */
	poly_vec_l_ntt(&s1);
	poly_vec_k_ntt(&u.s2);
	poly_vec_k_ntt_volatile(&N_storage.pack1.t0);
rej:
	/* Sample intermediate vector y */
	poly_vec_l_uniform_gamma1_volatile(&N_storage.y, rhoPrime, nonce); //TODO: fix this method
	nonce++;

// 	/* Matrix-vector multiplication */
	int32_t temp_val = 0;
	uint32_t counter = 0;
	for(int i = 0; i < L; i++) {
		memmove(&ubuf.temp_poly, &N_storage.z.vec[i], sizeof(Poly));
		for(int j = 0; j < N; j++) {
			temp_val = N_storage.y.vec[i].coeffs[j];
			ubuf.temp_poly.coeffs[j] = temp_val;
			// nvm_write((void*)&N_storage.z.vec[i].coeffs[j], &temp_val, sizeof(int32_t));counter++;
		}
		nvm_write((void*)&N_storage.z.vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("z=y %u\n", counter);
	poly_vec_l_ntt_volatile(&N_storage.z);
	combined_method(&N_storage.w1, &N_storage.z, &rho); //TODO: fix this method
	poly_vec_k_reduce_volatile(&N_storage.w1);
	poly_vec_k_inv_ntt_to_mont_volatile(&N_storage.w1);
	// /* Decompose w and call the random oracle */
	poly_vec_k_c_addq_volatile(&N_storage.w1);
	poly_vec_k_decompose_volatile(&N_storage.w1, &N_storage.w0, &N_storage.w1);
	poly_vec_k_pack_w1_volatile(&N_storage.sig, &N_storage.w1);

	shake256_init(&ctx);
	shake256_absorb(&ctx, mu, CRH_BYTES);
	shake256_absorb(&ctx, &N_storage.sig, K*POLY_W1_PACKED_BYTES);
	shake256_finalize(&ctx);
	shake256_squeeze_volatile(&ctx, &N_storage.sig, CTILDE_BYTES);
	shake256_clear(&ctx);
	poly_challenge_volatile(&N_storage.cp, &N_storage.sig, CTILDE_BYTES);
	poly_ntt_volatile(&N_storage.cp);

	// /* Compute z, reject if it reveals secret */
	poly_vec_l_pointwise_poly_montgomery_volatile(&N_storage.z, &N_storage.cp, &s1);
	poly_vec_l_inv_ntt_to_mont_volatile(&N_storage.z);
	poly_vec_l_add_volatile(&N_storage.z, &N_storage.z, &N_storage.y);
	poly_vec_l_reduce_volatile(&N_storage.z);
	if(poly_vec_l_chk_norm(&N_storage.z, GAMMA1-BETA) != 0) {
		goto rej;
	}

// 	/* Check that subtracting cs2 does not change high bits of w and low bits
// 	 * do not reveal secret information */
	poly_vec_k_pointwise_poly_montgomery_volatile(&N_storage.h, &N_storage.cp, &u.s2);
	poly_vec_k_inv_ntt_to_mont_volatile(&N_storage.h);
	poly_vec_k_sub_volatile(&N_storage.w0, &N_storage.w0, &N_storage.h);
	poly_vec_k_reduce_volatile(&N_storage.w0);
	if(poly_vec_k_chk_norm(&N_storage.w0, GAMMA2-BETA) != 0) {
		goto rej;
	}

	// /* Compute hints for w1 */
	poly_vec_k_pointwise_poly_montgomery_volatile(&N_storage.h, &N_storage.cp, &N_storage.pack1.t0);
	poly_vec_k_inv_ntt_to_mont_volatile(&N_storage.h);
	poly_vec_k_reduce_volatile(&N_storage.h);
	if(poly_vec_k_chk_norm(&N_storage.h, GAMMA2) != 0) {
		goto rej;
	}

	poly_vec_k_add_volatile(&N_storage.w0, &N_storage.w0, &N_storage.h);
	uint32_t n_ret = poly_vec_k_make_hint_volatile(&N_storage.h, &N_storage.w0, &N_storage.w1);
	if(n_ret > OMEGA) {
		goto rej;
	}
	uint8_t c[CTILDE_BYTES] = {0};
	for(int i = 0; i < CTILDE_BYTES; i++) {
		c[i] = N_storage.sig[i];
	}
	err = pack_sig_volatile(&N_storage.sig, CRYPTO_BYTES, c, &N_storage.z, &N_storage.h); 
	if(err != 0) {
		return err;
	}
	return 0;
}

ErrorCode crypto_sign_signature(uint8_t *sig, size_t sig_len, uint8_t *m, size_t m_len, uint8_t *ctx, size_t ctx_len, uint8_t (*sk)[CRYPTO_SECRET_KEY_BYTES], bool randomized_signing) {
    if(ctx_len > 255) {
		return ERR_INVALID_CONTEXT_LENGTH;
	}
	uint8_t rnd[RND_BYTES] = {0};
	uint8_t pre[CTX_LEN + 2] = {0};
	pre[0] = 0;
	pre[1] = (uint8_t)ctx_len;
	uint8_t *pre_ptr = pre + 2;
	copy_array(pre_ptr, ctx_len, ctx, ctx_len);

	if(randomized_signing) {
		cx_rng(rnd, sizeof(rnd));
	}

    pre_ptr = pre - 2;
	return crypto_sign_signature_internal(sig, sig_len, m, m_len, pre_ptr, ctx_len + 2, rnd, sk);
}

ErrorCode crypto_sign(uint8_t *msg, size_t msg_len, uint8_t *ctx, size_t ctx_len, uint8_t (*sk)[CRYPTO_SECRET_KEY_BYTES], bool randomizedSigning, uint8_t *sig) {
    sig = sig + CRYPTO_BYTES;
	copy_array(sig, msg_len, msg, msg_len);
    sig = sig - CRYPTO_BYTES;
	int err = crypto_sign_signature(sig, CRYPTO_BYTES, sig + CRYPTO_BYTES, msg_len, ctx, ctx_len, sk, randomizedSigning);
	return err;
}

ErrorCode crypto_sign_verify_internal(uint8_t sig[CRYPTO_BYTES], uint8_t *m [], size_t m_len, uint8_t *pre, size_t pre_len, uint8_t (*pk)[CRYPTO_PUBLIC_KEY_BYTES], bool *ret) {
	uint8_t buf[K * POLY_W1_PACKED_BYTES] = {0};
	uint8_t rho[SEED_BYTES] = {0};
	uint8_t mu[CRH_BYTES] = {0};
	uint8_t c[CTILDE_BYTES] = {0};
	uint8_t c2[CTILDE_BYTES] = {0};

	Poly cp;
    explicit_bzero(&cp, sizeof(cp));

	PolyVecL mat[K];
    explicit_bzero(mat, sizeof(mat));

	PolyVecL z;
    explicit_bzero(&z, sizeof(PolyVecL));

	PolyVecK t1, w1, h;
    explicit_bzero(&t1, sizeof(t1));
	explicit_bzero(&w1, sizeof(w1));
	explicit_bzero(&h, sizeof(h));

	unpack_pk(&rho, &t1, pk);
	if(unpack_sig(&c, &z, &h, sig) != 0) {
        *ret = false;
		return 0;
	}
	if(poly_vec_l_chk_norm(&z, GAMMA1-BETA) != 0) {
        *ret = false;
		return 0;
	}

	/* Compute CRH(H(rho, t1), pre, msg) */
	shake256_ctx ctx;
	shake256_init(&ctx);
	shake256_absorb(&ctx, (uint8_t*)pk, CRYPTO_PUBLIC_KEY_BYTES);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, mu, TR_BYTES);
	shake256_clear(&ctx);

	shake256_init(&ctx);
	shake256_absorb(&ctx, mu, TR_BYTES);
	shake256_absorb(&ctx, pre, pre_len);
	shake256_absorb(&ctx, *m, m_len);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, mu, CRH_BYTES);
	shake256_clear(&ctx);

	/* Matrix-vector multiplication; compute Az - c2^dt1 */
    int err = poly_challenge(&cp, c, CTILDE_BYTES);
	if (err != 0) {
        *ret = false;
		return err;
	}
    err = poly_vec_matrix_expand(&mat, &rho);
	if(err != 0) {
        *ret = false;
		return err;
	}

	poly_vec_l_ntt(&z);
	poly_vec_matrix_pointwise_montgomery(&w1, &mat, &z);

	poly_ntt(&cp);
	poly_vec_k_shift_l(&t1);
	poly_vec_k_ntt(&t1);
	poly_vec_k_pointwise_poly_montgomery(&t1, &cp, &t1);

	poly_vec_k_sub(&w1, &w1, &t1);
	poly_vec_k_reduce(&w1);
	poly_vec_k_inv_ntt_to_mont(&w1);

	/* Reconstruct w1 */
	poly_vec_k_c_addq(&w1);
	poly_vec_k_use_hint(&w1, &w1, &h);
    err = poly_vec_k_pack_w1(buf, K*POLY_W1_PACKED_BYTES, &w1);
	if(err != 0) {
        *ret = false;
		return err;
	}

	/* Call random oracle and verify challenge */
	shake256_init(&ctx);
	shake256_absorb(&ctx, mu, CRH_BYTES);
	shake256_absorb(&ctx, buf, K*POLY_W1_PACKED_BYTES);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, c2, CTILDE_BYTES);
	shake256_clear(&ctx);

	for(int i = 0; i < CTILDE_BYTES; i++) {
		if(c[i] != c2[i]) {
            *ret = false;
			return 0;
		}
	}

    *ret = true;
	return 0;
}

static int32_t unpack_sig_volatile(uint8_t (*c)[CTILDE_BYTES],
	volatile PolyVecL *z,
	volatile PolyVecK *h,
	volatile uint8_t sig[CRYPTO_BYTES]) {
	uint32_t counter = 0;
	copy_array((uint8_t *)c, CTILDE_BYTES, sig, CTILDE_BYTES);

	int32_t temp = 0;
	for(int b = 0; b < L; b++) {
		memmove(&ubuf.temp_poly, &z->vec[b], sizeof(Poly));
		for(int i = 0; i < N/2; i++) {
			temp = (int32_t)(sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES + 5*i+0]);
			ubuf.temp_poly.coeffs[2*i+0] = temp;
			// nvm_write((void *)&z->vec[b].coeffs[2*i+0], &temp, sizeof(int32_t));counter++;
			// z->vec[b].coeffs[2*i+0] = (int32_t)(sig[5*i+0]);
			temp = ubuf.temp_poly.coeffs[2*i+0] | (int32_t)((uint32_t)(sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES + 5*i+1]) << 8);
			ubuf.temp_poly.coeffs[2*i+0] = temp;
			// nvm_write((void *)&z->vec[b].coeffs[2*i+0], &temp, sizeof(int32_t));counter++;
			// z->vec[b].coeffs[2*i+0] |= (int32_t)((uint32_t)(sig[5*i+1]) << 8);
			temp = ubuf.temp_poly.coeffs[2*i+0] | (int32_t)((uint32_t)(sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES + 5*i+2]) << 16);
			ubuf.temp_poly.coeffs[2*i+0] = temp;
			// nvm_write((void *)&z->vec[b].coeffs[2*i+0], &temp, sizeof(int32_t));counter++;
			// z->vec[b].coeffs[2*i+0] |= (int32_t)((uint32_t)(sig[5*i+2]) << 16);
			temp = ubuf.temp_poly.coeffs[2*i+0] & 0xFFFFF;
			ubuf.temp_poly.coeffs[2*i+0] = temp;
			// nvm_write((void *)&z->vec[b].coeffs[2*i+0], &temp, sizeof(int32_t));counter++;
			// z->vec[b].coeffs[2*i+0] &= 0xFFFFF;

			temp = (int32_t)(sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES + 5*i+2] >> 4);
			ubuf.temp_poly.coeffs[2*i+1] = temp;
			// nvm_write((void *)&z->vec[b].coeffs[2*i+1], &temp, sizeof(int32_t));counter++;
			// z->vec[b].coeffs[2*i+1] = (int32_t)(sig[5*i+2] >> 4);
			temp = ubuf.temp_poly.coeffs[2*i+1] | (int32_t)((uint32_t)(sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES + 5*i+3]) << 4);
			ubuf.temp_poly.coeffs[2*i+1] = temp;
			// nvm_write((void *)&z->vec[b].coeffs[2*i+1], &temp, sizeof(int32_t));counter++;
			// z->vec[b].coeffs[2*i+1] |= (int32_t)((uint32_t)(sig[5*i+3]) << 4);
			temp = ubuf.temp_poly.coeffs[2*i+1] | (int32_t)((uint32_t)(sig[CTILDE_BYTES + b*POLY_Z_PACKED_BYTES + 5*i+4]) << 12);
			ubuf.temp_poly.coeffs[2*i+1] = temp;
			// nvm_write((void *)&z->vec[b].coeffs[2*i+1], &temp, sizeof(int32_t));counter++;
			// z->vec[b].coeffs[2*i+1] |= (int32_t)((uint32_t)(sig[5*i+4]) << 12);
			temp = ubuf.temp_poly.coeffs[2*i+0] & 0xFFFFF;
			ubuf.temp_poly.coeffs[2*i+0] = temp;
			// nvm_write((void *)&z->vec[b].coeffs[2*i+0], &temp, sizeof(int32_t));counter++;
			// z->vec[b].coeffs[2*i+0] &= 0xFFFFF; // TODO (cyyber): This line has no use, might be removed

			temp = GAMMA1 - ubuf.temp_poly.coeffs[2*i+0];
			ubuf.temp_poly.coeffs[2*i+0] = temp;
			// nvm_write((void *)&z->vec[b].coeffs[2*i+0], &temp, sizeof(int32_t));counter++;
			// z->vec[b].coeffs[2*i+0] = GAMMA1 - z->vec[b].coeffs[2*i+0];
			temp = GAMMA1 - ubuf.temp_poly.coeffs[2*i+1];
			ubuf.temp_poly.coeffs[2*i+1] = temp;
			// nvm_write((void *)&z->vec[b].coeffs[2*i+1], &temp, sizeof(int32_t));counter++;
			// z->vec[b].coeffs[2*i+1] = GAMMA1 - z->vec[b].coeffs[2*i+1];
		}
		nvm_write((void*)&z->vec[b], &ubuf.temp_poly, sizeof(Poly));counter++;
	}

	/* Decode h */
	uint32_t k = (uint32_t)0;
	for(int i = 0; i < K; i++) {  
		for(int j = 0; j < N; j++) {
			temp = 0;
			nvm_write((void *)&h->vec[i].coeffs[j], &temp, sizeof(int32_t));counter++;
		}
		if((uint32_t)(sig[CTILDE_BYTES + L*POLY_Z_PACKED_BYTES + OMEGA+i]) < k || sig[CTILDE_BYTES + L*POLY_Z_PACKED_BYTES + OMEGA+i] > OMEGA) {
			return 1;
		}
		for(uint32_t j = k; j < (uint32_t)(sig[CTILDE_BYTES + L*POLY_Z_PACKED_BYTES + OMEGA+i]); j++) {
			/* Coefficients are ordered for strong unforgeability */
			if(j > k && sig[CTILDE_BYTES + L*POLY_Z_PACKED_BYTES + j] <= sig[CTILDE_BYTES + L*POLY_Z_PACKED_BYTES + j-1]) {
				return 1;
			}
			temp = 1;
			nvm_write((void *)&h->vec[i].coeffs[sig[CTILDE_BYTES + L*POLY_Z_PACKED_BYTES + j]], &temp, sizeof(int32_t));counter++;
			// h->vec[i].coeffs[sig[j]] = 1;
		}
		k = (uint32_t)(sig[CTILDE_BYTES + L*POLY_Z_PACKED_BYTES + OMEGA+i]);
	}

	for(int j = k; j < OMEGA; j++) {
		if(sig[CTILDE_BYTES + L*POLY_Z_PACKED_BYTES + j] != 0) {
			return 1;
		}
	}
	PRINTF("unpack_sig_volatile %u\n", counter);
	return 0;
}

static void poly_vec_k_shift_l_volatile(volatile PolyVecK *v) {
	uint32_t counter = 0;
	int32_t temp = 0;
    for(int i = 0; i < K; i++) {
		memmove(&ubuf.temp_poly, &v->vec[i], sizeof(Poly));
		for(int j = 0; j < N; j++) {
			temp = ubuf.temp_poly.coeffs[j] << D;
			ubuf.temp_poly.coeffs[j] = temp;
			// nvm_write((void *)&v->vec[i].coeffs[j], &temp, sizeof(int32_t));counter++;
			// a->coeffs[i] <<= D;
		}
		nvm_write((void*)&v->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_k_shift_l_volatile %u\n", counter);
}


static void poly_vec_k_use_hint_volatile(volatile PolyVecK *w, volatile PolyVecK *u, volatile PolyVecK *h) {
	uint32_t counter = 0;
	//wuv, bah
	//hint = h
	int32_t temp = 0;
    for(int i = 0; i < K; i++) {
		memmove(&ubuf.temp_poly, &w->vec[i], sizeof(Poly));
		for(int j = 0; j < N; j++) {
			int32_t a0 = 0, a1 = 0;
			int32_t a = u->vec[i].coeffs[j];
			int32_t ret = 0;

			int32_t a_decompose = (a + 127) >> 7;
			a_decompose = (a_decompose*1025 + (1 << 21)) >> 22;
			a_decompose &= 15;

			a0 = a - a_decompose*2*GAMMA2;
			a0 -= (((Q_CONST-1)/2 - a0) >> 31) & Q_CONST;

			a1 = a_decompose;
			if(h->vec[i].coeffs[j] == 0) {
				ret = a1;
			} else if(a0 > 0) {
				ret = (a1 + 1) & 15;
			} else {
				ret = (a1 - 1) & 15;
			}
			temp = ret;
			ubuf.temp_poly.coeffs[j] = ret;
			// nvm_write((void *)&w->vec[i].coeffs[j], &temp, sizeof(int32_t));counter++;
			// b->coeffs[i] = ret;
		}
		nvm_write((void*)&w->vec[i], &ubuf.temp_poly, sizeof(Poly));counter++;
	}
	PRINTF("poly_vec_k_use_hint_volatile %u\n", counter);
}

typedef union {
	PolyVecK s2;
	PolyVecK t0;
} union_s2_t0;

ErrorCode crypto_verify_optimized(const uint32_t bip32_path[], size_t bip32_path_len, uint8_t *sig, size_t sig_len, uint8_t *message, size_t message_len, bool *is_verified) {
	//Data validation
	if(sig == NULL || sig_len == 0) {
		*is_verified = false;
		return ERR_INVALID_SIGNATURE;
	}
	if(message == NULL || message_len == 0) {
		*is_verified = false;
		return ERR_INVALID_MESSAGE;
	}
	if(CTX_LEN > 255) {
		*is_verified = false;
		return ERR_INVALID_CONTEXT_LENGTH;
	}
	if(sig_len != CRYPTO_BYTES) {
		*is_verified = false;
		return ERR_INVALID_SIGNATURE;
	}

	//Message verify prepare
	uint8_t raw_seed[64] = {0};
    cx_err_t err = os_derive_bip32_no_throw(
        CX_CURVE_SECP256K1,
        bip32_path,
        bip32_path_len,
        raw_seed,
        NULL
    );
    if(err != 0) {
		*is_verified = false;
        return -1;
    }
    uint8_t seed[32] = {0};
    for(int i = 0; i < 32; i++) {
        seed[i] = raw_seed[i];
    }

	uint8_t rnd[RND_BYTES] = {0};
	uint8_t pre[CTX_LEN + 2] = {0};
	pre[0] = 0;
	pre[1] = (uint8_t)CTX_LEN;
	for(int i = 0; i < CTX_LEN; ++i) {
		pre[i+2] = CTX[i];
	}

	//From keypair method
	uint8_t tr[TR_BYTES] = {0};
	uint8_t rho[SEED_BYTES] = {0}; 
	uint8_t key[SEED_BYTES] = {0}; 
	uint8_t rhoPrime[CRH_BYTES] = {0};

	PolyVecL s1;
	union_s2_t0 u;

	/* Expand 32 bytes of randomness into rho, rhoprime and key */
	shake256_ctx ctx;
	shake256_init(&ctx);
	shake256_absorb(&ctx, seed, SEED_BYTES);
	uint8_t extraData[2] = {K, L};
	shake256_absorb(&ctx, extraData, 2);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, rho, SEED_BYTES);
	shake256_squeeze(&ctx, rhoPrime, CRH_BYTES);
	shake256_squeeze(&ctx, key, SEED_BYTES);
	shake256_clear(&ctx);

	/* Sample short vectors s1 and s2 */
	err = poly_vec_l_uniform_eta(&s1, &rhoPrime, 0); 
	if(err != 0) {
		return err;
	}
	err = poly_vec_k_uniform_eta(&u.s2, &rhoPrime, L); 
	if(err != 0) {
		return err;
	}

	/* Matrix-vector multiplication */
	// u.s1hat = s1;
	poly_vec_l_ntt(&s1);
	combined_method(&N_storage.t1, &s1, rho);
	poly_vec_k_reduce_volatile(&N_storage.t1);
	poly_vec_k_inv_ntt_to_mont_volatile(&N_storage.t1);
	/* Add noise vector s2 */
	poly_vec_k_add_volatile(&N_storage.t1, &N_storage.t1, &u.s2);
	/* Extract t1 and write public key */
	poly_vec_k_c_addq_volatile(&N_storage.t1);
	poly_vec_k_power2_round_volatile(&N_storage.t1, &N_storage.pack1.t0, &N_storage.t1);
	pack_pk_volatile(&N_storage.pk, &rho, &N_storage.t1);
	
	//From verify method
	// uint8_t buf[K * POLY_W1_PACKED_BYTES] = {0};
	uint8_t mu[CRH_BYTES] = {0};
	uint8_t c[CTILDE_BYTES] = {0};
	uint8_t c2[CTILDE_BYTES] = {0};

	if(unpack_sig_volatile(&c, &N_storage.z, &N_storage.h, sig) != 0) {
        *is_verified = false;
		return 0;
	}
	
	if(poly_vec_l_chk_norm(&N_storage.z, GAMMA1-BETA) != 0) {
        *is_verified = false;
		return 0;
	}

	/* Compute CRH(H(rho, t1), pre, msg) */
	// shake256_ctx ctx;
	shake256_init(&ctx);
	shake256_absorb(&ctx, &N_storage.pk, CRYPTO_PUBLIC_KEY_BYTES);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, mu, TR_BYTES);
	shake256_clear(&ctx);

	shake256_init(&ctx);
	shake256_absorb(&ctx, mu, TR_BYTES);
	shake256_absorb(&ctx, pre, CTX_LEN+2);
	shake256_absorb(&ctx, message, message_len);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, mu, CRH_BYTES);
	shake256_clear(&ctx);

	/* Matrix-vector multiplication; compute Az - c2^dt1 */
    err = poly_challenge_volatile(&N_storage.cp, c, CTILDE_BYTES);
	if (err != 0) {
        *is_verified = false;
		return err;
	}

	poly_vec_l_ntt_volatile(&N_storage.z);
	// poly_vec_matrix_expand(&mat, &rho);
	// poly_vec_matrix_pointwise_montgomery(&w1, &mat, &z);
	combined_method(&N_storage.w1, &N_storage.z, &rho);

	poly_ntt_volatile(&N_storage.cp);
	poly_vec_k_shift_l_volatile(&N_storage.t1);
	poly_vec_k_ntt_volatile(&N_storage.t1);
	poly_vec_k_pointwise_poly_montgomery_volatile(&N_storage.t1, &N_storage.cp, &N_storage.t1);

	poly_vec_k_sub_volatile(&N_storage.w1, &N_storage.w1, &N_storage.t1);
	poly_vec_k_reduce_volatile(&N_storage.w1);
	poly_vec_k_inv_ntt_to_mont_volatile(&N_storage.w1);

	/* Reconstruct w1 */
	poly_vec_k_c_addq_volatile(&N_storage.w1);
	poly_vec_k_use_hint_volatile(&N_storage.w1, &N_storage.w1, &N_storage.h);
    poly_vec_k_pack_w1_volatile_verify(&N_storage.pack1.buf_verify, &N_storage.w1);

	/* Call random oracle and verify challenge */
	shake256_init(&ctx);
	shake256_absorb(&ctx, mu, CRH_BYTES);
	shake256_absorb(&ctx, N_storage.pack1.buf_verify, K*POLY_W1_PACKED_BYTES);
	shake256_finalize(&ctx);
	shake256_squeeze(&ctx, c2, CTILDE_BYTES);
	shake256_clear(&ctx);

	for(int i = 0; i < CTILDE_BYTES; i++) {
		if(c[i] != c2[i]) {
            *is_verified = false;
			return ERR_NONE;
		}
	}

    *is_verified = true;
	return ERR_NONE;
}

ErrorCode crypto_sign_verify(uint8_t sig[CRYPTO_BYTES], uint8_t *m, size_t m_len, uint8_t *ctx, size_t ctx_len, uint8_t (*pk)[CRYPTO_PUBLIC_KEY_BYTES], bool *ret) {
    if(ctx_len > 255) {
		*ret = false;
		return ERR_INVALID_CONTEXT_LENGTH;
	}
	uint8_t pre[CTX_LEN + 2] = {0};
	pre[0] = 0;
	pre[1] = (uint8_t)ctx_len;
	uint8_t *pre_ptr = pre + 2;
	copy_array(pre_ptr, CTX_LEN, ctx, CTX_LEN);
	pre_ptr = pre-2;
	bool result = false;
	return crypto_sign_verify_internal(sig, &m, m_len, pre_ptr, ctx_len+2, pk, &result);
}

ErrorCode crypto_sign_open(uint8_t *sm, size_t sm_len, uint8_t *ctx, size_t ctx_len, uint8_t (*pk)[CRYPTO_PUBLIC_KEY_BYTES], uint8_t *sig) {
	if (sm_len < CRYPTO_BYTES) {
		sig = NULL;
		return ERR_INVALID_LENGTH;
	}
	uint8_t *msg[MESSAGE_LEN] = {0};

	copy_array(sig, CRYPTO_BYTES, sm, CRYPTO_BYTES);
	copy_array(msg, sm_len - CRYPTO_BYTES, sm, sm_len - CRYPTO_BYTES);

	bool result = false;
	int err = crypto_sign_verify(sig, msg, sm_len - CRYPTO_BYTES, ctx, ctx_len, pk, &result); 

	if(err != 0 || !result) {
		sig = NULL;
		return err;
	}

	sig = msg;
	return ERR_NONE;
}