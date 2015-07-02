#include "cripto_rand.h"
#include <stdlib.h>

void randomize(int num)
{
	srand((int) num);
}

double randnormalize(void)
{
	return rand() / ((double) RAND_MAX + 1);
}

long int randint(long int max)
{
	/* Devuelve un numero en [0, max] */
	return (long int) (randnormalize() * (max + 1));
}