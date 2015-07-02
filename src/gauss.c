#include "gauss.h"
#include "recover.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

bmp_op_t mul_inverse_calc(bmp_op_t num, bmp_op_t mod)
{
	bmp_op_t b0 = mod, t, q;
	bmp_op_t x0 = 0, x1 = 1;
	if (mod == 1)
	{
		return 1;
	}

	while (num > 1)
	{
		q = num / mod;
		t = mod;
		mod = num % mod;
		num = t;
		t = x0;
		x0 = x1 - q * x0;
		x1 = t;
	}

	if (x1 < 0)
	{
		x1 += b0;
	}
	return x1;
}

bmp_op_t mul_inverse(bmp_op_t num)
{
	static bmp_op_t store[251] = {0};
	if (store[num])
	{
		return store[num];
	}

	bmp_op_t val = mul_inverse_calc(num, 251);
	store[num] = val;
	return val;
}

bmp_op_t restrain_mod(bmp_op_t num)
{
	num = num % 251;
	if (num < 0)
	{
		num += 251;
	}
	return num;
}

int recover_gauss(bmp_op_t **original, bmp_byte_t *results, size_t n)
{
	int i, j, k;
	bmp_op_t c;

	bmp_op_t **matrix = recover_alloc_matrix(n);
	if (matrix == NULL)
	{
		return -1;
	}

	memcpy(matrix[0], original[0], n * (n + 1) * sizeof(bmp_op_t));

	for (i = 0; i < n; i++)
	{
		for (j = 0; j < n; j++)
		{
			if (j != i)
			{
				c = matrix[j][i] * mul_inverse(matrix[i][i]);
				c = restrain_mod(c);

				for (k = 0; k < n + 1; k++)
				{
					bmp_op_t aux = matrix[j][k] - (c * matrix[i][k]);
					matrix[j][k] = restrain_mod(aux);
				}
			}
		}
	}

	for (i = 0; i < n; i++)
	{
		bmp_op_t result = matrix[i][n] * mul_inverse(matrix[i][i]);
		results[i] = (bmp_byte_t)restrain_mod(result);
	}

	free(matrix[0]);
	free(matrix);
	return 0;
}