#include "bmp.h"
#include "recover.h"
#include "utils.h"
#include "gauss.h"
#include "cripto.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

bmp_byte_t recover_byte(bmp_byte_t *bytes, size_t k);
void recover_generate_equations(bmp_op_t ** matrix, size_t k, struct bmp_handle **shadows);
bmp_byte_t recover_byte(bmp_byte_t *bytes, size_t k);
bmp_byte_t recover_lsb_width1(bmp_byte_t *bytes, size_t pos);
bmp_byte_t recover_lsb_width2(bmp_byte_t *bytes, size_t pos);

int recover(struct bmp_handle *bmp, struct bmp_handle **shadows, size_t k, int permute)
{
	if (bmp == NULL || shadows == NULL || k < MIN_K)
	{
		return -1;
	}

	bmp_byte_t *coefs = malloc(k * sizeof(bmp_byte_t));
	if (coefs == NULL)
	{
		return -1;
	}

	bmp_byte_t *new_bmp_pixels = bmp_get_pixels(bmp);

	bmp_op_t **equations = recover_alloc_matrix(k);
	recover_generate_equations(equations, k, shadows);
	struct bmp_header *header = bmp_get_header(bmp);
	int jump;
	bmp_byte_t (*recover_lsb_fn)(bmp_byte_t*, size_t);

	if (k >= 8)
	{
		jump = 8;
		recover_lsb_fn = recover_lsb_width1;
	}
	else
	{
		jump = 4;
		recover_lsb_fn = recover_lsb_width2;
	}

	int padding = padding_for_width(header->width);
	size_t real_byte_count = (header->width + padding) * header->height;

	int remainder = real_byte_count % k;
	int i;
	size_t bytes_written = 0;

	for (i = 0; i < (real_byte_count - remainder) / k; i++)
	{
		int j;
		for (j = 0; j < k; j++)
		{
			bmp_byte_t *pixels = bmp_get_pixels(shadows[j]);
			equations[j][k] = (*recover_lsb_fn)(&pixels[i * jump], LSB_POS_1);
		}

		if (recover_gauss(equations, coefs, k) != 0)
		{
			bmp_free(bmp);
			free(equations);
			free(coefs);
			return -1;
		}

		memcpy(&new_bmp_pixels[(i * k)], coefs, k);
		bytes_written += k;
	}

	if (remainder)
	{
		printv("Info: real_byte_count (mod K) = %d\n", remainder);

		size_t lsb_pos;
		if (k >= 8)
		{
			lsb_pos = LSB_POS_2;
		}
		else
		{
			lsb_pos = LSB_POS_3;
		}

		int j;
		for (j = 0; j < k; j++)
		{
			bmp_byte_t *pixels = bmp_get_pixels(shadows[j]);
			equations[j][k] = recover_lsb_width1(pixels, lsb_pos);
		}

		if (recover_gauss(equations, coefs, k) != 0)
		{
			bmp_free(bmp);
			free(equations);
			free(coefs);
			return -1;
		}

		memcpy(&new_bmp_pixels[(i * k)], coefs, remainder);
		bytes_written += remainder;
	}

	printv("Bytes written to recovered image: %u\n", bytes_written);

	if (permute)
	{
		utils_permute_inverse(new_bmp_pixels, real_byte_count, (bmp_get_header(shadows[0]))->seed);
	}
	int status = bmp_write_pixels(bmp);
	if (status)
	{
		bmp_free(bmp);
		free(equations);
		free(coefs);
		return -1;
	}

	free(equations);
	free(coefs);
	return 0;
}

bmp_byte_t recover_lsb_width1(bmp_byte_t *bytes, size_t pos)
{
	int i;
	bmp_byte_t byte = 0;
	for (i = 0; i < 8; i++)
	{
		bmp_byte_t bit = bytes[i] & BIT(pos);
		bit = bit >> pos;
		byte |= (bit << (8 - 1 - i));
	}

	return byte;
}

bmp_byte_t recover_lsb_width2(bmp_byte_t *bytes, size_t pos)
{
	int i;
	bmp_byte_t byte = 0;
	for (i = 0; i < 4; i++)
	{
		bmp_byte_t bits = bytes[i] & (0x03 << pos);
		bits = bits >> pos;
		byte |= (bits << (8 - 2 - (i * 2)));
	}

	return byte;
}

bmp_op_t **recover_alloc_matrix(size_t k)
{
	bmp_op_t **rows = malloc(k * sizeof(bmp_op_t*));

	if (rows == NULL)
	{
		return NULL;
	}

	bmp_op_t *col_data = malloc(k * (k + 1) * sizeof(bmp_op_t));

	if (col_data == NULL)
	{
		return NULL;
	}

	int i;
	for (i = 0; i < k; i++)
	{
		rows[i] = &col_data[i * (k + 1)];
	}

	return rows;
}

void recover_generate_equations(bmp_op_t ** matrix, size_t k, struct bmp_handle **shadows)
{
	int i, j;
	struct bmp_header *header;
	for (i = 0; i < k; i++)
	{
		header = bmp_get_header(shadows[i]);
		for (j = 0; j < k; j++)
		{
			matrix[i][j] = ((bmp_op_t) pow(header->shadow_index, j)) % 251;
		}
	}
}
