#include "distribute.h"
#include "utils.h"
#include "cripto_rand.h"
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

void distribute_truncate_image(bmp_byte_t *pixels, size_t size);
bmp_word_t distribute_gen_seed();
bmp_byte_t distribute_evaluate_pol(bmp_byte_t *pol, size_t size, bmp_byte_t x);
void distribute_lsb_width1(bmp_byte_t byte, bmp_byte_t *pixels, size_t pos);
void distribute_lsb_width2(bmp_byte_t byte, bmp_byte_t *pixels, size_t pos);

int distribute(struct bmp_handle *secret_bmp, struct bmp_handle **shadows, size_t n, size_t k, int permute)
{
	bmp_byte_t *pixels = bmp_get_pixels(secret_bmp);
	struct bmp_header *header = bmp_get_header(secret_bmp);

	int padding = padding_for_width(header->width);
	size_t real_byte_count = (header->width + padding) * header->height;

	distribute_truncate_image(pixels, real_byte_count);

	bmp_byte_t seed = distribute_gen_seed();

	if (permute)
	{
		utils_permute(pixels, real_byte_count, seed);
	}

	int i, j, remainder;
	bmp_byte_t byte;
	bmp_byte_t *shadow_pixels;

	remainder = real_byte_count % k;

	void (*lsb_fn_ptr)(bmp_byte_t, bmp_byte_t*, size_t);
	int jump;

	if (k >= 8)
	{
		jump = 8;
		lsb_fn_ptr = distribute_lsb_width1;
	}
	else
	{
		jump = 4;
		lsb_fn_ptr = distribute_lsb_width2;
	}

	size_t bytes_written = 0;

	for (i = 0; i < (real_byte_count - remainder) / k; i++)
	{
		for (j = 0; j < n; j++)
		{
			byte = distribute_evaluate_pol(pixels + (i * k), k, j + 1);
			shadow_pixels = bmp_get_pixels(shadows[j]);
			(*lsb_fn_ptr)(byte, shadow_pixels + (i * jump), LSB_POS_1);
		}

		bytes_written += jump;
	}

	printv("Bytes written to each shadow (first layer): %u\n", bytes_written);

	if (remainder) // some pixels were left over
	{
		size_t lsb_pos;
		if (k >= 8)
		{
			lsb_pos = LSB_POS_2;
		}
		else
		{
			lsb_pos = LSB_POS_3;
		}

		printv("Info: real_byte_count (mod K) = %d\n", remainder);

		bmp_byte_t *extra_pixels = malloc(k * sizeof(bmp_byte_t));
		if (extra_pixels == NULL)
		{
			return -1;
		}

		memcpy(extra_pixels, &pixels[real_byte_count - remainder], remainder * sizeof(bmp_byte_t));
		randomize(time(NULL));
		for (i = remainder; i < k; i++)
		{
			extra_pixels[i] = (bmp_byte_t)randint(250);
		}

		for (j = 0; j < n; j++)
		{
			byte = distribute_evaluate_pol(extra_pixels, k, j + 1);
			shadow_pixels = bmp_get_pixels(shadows[j]);
			distribute_lsb_width1(byte, shadow_pixels, lsb_pos);
		}

		printv("Bytes written to each shadow (second layer): 8\n");
	}

	// write header and pixels
	for (i = 0; i < n; i++)
	{
		header = bmp_get_header(shadows[i]);
		header->seed = seed;
		header->shadow_index = i + 1;
		bmp_write_header(shadows[i]);
		bmp_write_pixels(shadows[i]);
	}

	return 0;
}

void distribute_lsb_width1(bmp_byte_t byte, bmp_byte_t *pixels, size_t pos)
{
	int i;
	for (i = 0; i < 8; i++)
	{
		pixels[i] = (pixels[i] & ~BIT(pos)) | (GET_BIT(byte, 8 - 1 - i) << pos);
	}
}

void distribute_lsb_width2(bmp_byte_t byte, bmp_byte_t *pixels, size_t pos)
{
	int j;
	for (j = 0; j < 8; j += 2)
	{
		int i = j / 2;
		bmp_byte_t to_save = (GET_BIT(byte, 8 - 1 - j) << 1) | GET_BIT(byte, 8 - 1 - (j + 1));
		pixels[i] = (pixels[i] & ~(BIT(pos + 1) | BIT(pos))) | (to_save << pos);
	}
}

void distribute_truncate_image(bmp_byte_t *pixels, size_t size)
{
	int i;
	for (i = 0; i < size; i++)
	{
		if (pixels[i] > 250)
		{
			pixels[i] = 250;
		}
	}
}

bmp_word_t distribute_gen_seed()
{
	randomize(time(NULL));
	return (bmp_word_t) randint(USHRT_MAX);
}

bmp_byte_t distribute_evaluate_pol(bmp_byte_t *pol, size_t size, bmp_byte_t x)
{
	int i, aux = 0;
	for (i = 0; i < size; i++)
	{
		aux += (pol[i] * ((long)pow(x, i) % 251)) % 251;
	}

	aux %= 251;
	return (bmp_byte_t)aux;
}
