#include "utils.h"
#include "cripto_rand.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

int *utils_generate_rand_numbers(size_t size, bmp_word_t seed);
void utils_swap(bmp_byte_t *array, int i, int j);

int utils_permute(bmp_byte_t *pixels, size_t size, bmp_word_t seed)
{
	int *rand_numbers = utils_generate_rand_numbers(size, seed);
	if (rand_numbers == NULL)
	{
		return -1;
	}

	int i;
	for (i = size - 1; i >= 0; i--)
	{
		utils_swap(pixels, i, rand_numbers[size - 1 - i]);
	}

	free(rand_numbers);
	return 0;
}

int utils_permute_inverse(bmp_byte_t *pixels, size_t size, bmp_word_t seed)
{
	int *rand_numbers = utils_generate_rand_numbers(size, seed);
	if (rand_numbers == NULL)
	{
		return -1;
	}

	int i;
	for (i = 0; i < size; i++)
	{
		utils_swap(pixels, i, rand_numbers[size - 1 - i]);
	}
	free(rand_numbers);
	return 0;
}

int *utils_generate_rand_numbers(size_t size, bmp_word_t seed)
{
	if (size == 0)
	{
		return NULL;
	}

	int *rand_numbers = malloc(size * sizeof(int));
	if (rand_numbers == NULL)
	{
		return NULL;
	}

	randomize(seed);

	int i;
	for (i = 0; i < size; i++)
	{
		rand_numbers[i] = randint(size -1);
	}

	return rand_numbers;
}

void utils_swap(bmp_byte_t *array, int i, int j)
{
	bmp_byte_t aux = array[i];
	array[i] = array[j];
	array[j] = aux;
}

int printv(const char *fmt, ...)
{
	if (!verbose_mode)
	{
		return 0;
	}

	va_list args;
	va_start(args, fmt);
	int ret = vprintf(fmt, args);
	va_end(args);
	return ret;
}

size_t shadow_size_for(bmp_dword_t image_size, int k)
{
	int lsb_bytes = (k >= 8 ? 8 : 4);
	size_t shadow_base_size = (image_size - (image_size % k)) / k;
	return shadow_base_size * lsb_bytes;
}

int padding_for_width(bmp_dword_t width)
{
	double quarter_width = (double)width / 4;
	return ((int)ceil(quarter_width) * 4) - width;
}