#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include "bmp.h"

extern int verbose_mode;

#define printe(...) fprintf(stderr, __VA_ARGS__)

#define BIT(n) (0x01 << (n))
#define GET_BIT(x, n) (((x) & (0x01 << (n))) >> (n))

#define LSB_POS_1 0
#define LSB_POS_2 1
#define LSB_POS_3 2

int utils_permute_inverse(bmp_byte_t *pixels, size_t size, bmp_word_t seed);
int utils_permute(bmp_byte_t *pixels, size_t size, bmp_word_t seed);
int printv(const char *fmt, ...);

int padding_for_width(bmp_dword_t width);

size_t shadow_size_for(bmp_dword_t image_size, int k);

#endif
/* UTILS_H */