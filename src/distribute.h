#ifndef DISTRIBUTE_H
#define DISTRIBUTE_H

#include "bmp.h"

int distribute(struct bmp_handle *secret_bmp, struct bmp_handle **shadows, size_t n, size_t k, int permute);

#endif
/* DISTRIBUTE_H */
