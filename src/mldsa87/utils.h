#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdint.h>
#include "error.h"
#include "polyvec.h"

void copy_array(uint8_t *dst, size_t dst_len, uint8_t *src, size_t src_len);
int decode_from_hex_string(const char *hex_str, uint8_t *out_bytes, size_t max_len);
void print_polyveck(PolyVecK *a);
void print_polyvecl(PolyVecL *a);
void print_poly(Poly *a);
void print_hex(char *, uint8_t *, size_t);

#endif // !UTILS_H