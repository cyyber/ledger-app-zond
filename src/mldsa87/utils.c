#include "utils.h"
#include <string.h>

void copy_array(uint8_t *dst, size_t dst_len, uint8_t *src, size_t src_len) {
    size_t len = dst_len < src_len ? dst_len : src_len;
    memmove(dst, src, len);
}
