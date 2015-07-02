#ifndef RECOVER_H
#define RECOVER_H

#include "bmp.h"

int recover(struct bmp_handle *bmp, struct bmp_handle **shadows, size_t k, int permute);
bmp_op_t **recover_alloc_matrix(size_t k);

#endif
/* RECOVER_H */