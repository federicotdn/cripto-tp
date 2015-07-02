#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "cripto.h"
#include "bmp.h"
#include "utils.h"

#include "recover.h"
#include "distribute.h"

#define TRUE 1
#define FALSE !TRUE

#define RECOVER_MODE 1
#define DISTRIBUTE_MODE 2
#define MAX_FILENAME_LEN 255
#define DEFAULT_DIR "."

int verbose_mode = FALSE;

enum cmd_status {
	CMD_SUCCESS, ERROR_D_AND_R, ERROR_NOMODE, ERROR_NOK,
	ERROR_NOSECRET, ERROR_GETOPT, ERROR_NON, ERROR_NODIR, ERROR_ORDER
};

struct cmd_options {
	int mode;
	int k;
	int n;
	char secret[MAX_FILENAME_LEN];
	char dir[MAX_FILENAME_LEN];
	bmp_dword_t secret_width;
	bmp_dword_t secret_height;
	int enable_permute;
};

int arg_invalid_pos(int c, int pos)
{
	return ((c == 'd' && pos != 0) ||
			(c == 'r' && pos != 0) ||
			(c == 's' && pos != 1) ||
			(c == 'k' && pos != 2) ||
			(c == 'n' && pos != 3) ||
			(c == 'i' && (pos != 3 && pos != 4)));
}

enum cmd_status parse_args(int argc, char *argv[], struct cmd_options *options)
{
 	options->mode = 0;
 	options->secret[0] = 0;
 	options->dir[0] = 0;
 	options->k = 0;
 	options->n = 0;
 	options->secret_width = 0;
 	options->secret_height = 0;
 	options->enable_permute = TRUE;

	static struct option long_options[] =
    {
		{"secret",  required_argument, NULL, 's'},
		{"dir",  required_argument, NULL, 'i'},
		{"verbose", no_argument, NULL, 'v'},
		{"no-permute", no_argument, NULL, 'p'},
		{NULL, 0, NULL, 0}
	};

	int option_pos = 0;

	while (1)
	{
		int c;
		size_t len;

		c = getopt_long_only(argc, argv, "drk:n:w:h:", long_options, NULL);

		if (c == -1)
		{
			break;
		}

		if (arg_invalid_pos(c, option_pos))
		{
			return ERROR_ORDER;
		}

		switch (c)
		{
			case 'd':
				if (options->mode)
				{
					return ERROR_D_AND_R;
				}
				options->mode = DISTRIBUTE_MODE;
			break;

			case 'r':
				if (options->mode)
				{
					return ERROR_D_AND_R;
				}
				options->mode = RECOVER_MODE;
			break;

			case 's':
				len = strlen(optarg);
				if (len > MAX_FILENAME_LEN - 1 || len == 0)
				{
					return ERROR_NOSECRET;
				}

				strcpy(options->secret, optarg);
			break;

			case 'i':
				len = strlen(optarg);
				if (len > MAX_FILENAME_LEN - 1 || len == 0)
				{
					return ERROR_NODIR;
				}

				strcpy(options->dir, optarg);
			break;

			case 'k':
				options->k = atoi(optarg);
			break;

			case 'n':
				options->n = atoi(optarg);
				if (options->n == 0)
				{
					return ERROR_NON;
				}
			break;

			case 'v':
				verbose_mode = TRUE;
			break;

			case 'w':
				options->secret_width = atoi(optarg);
			break;

			case 'h':
				options->secret_height = atoi(optarg);
			break;

			case 'p':
				options->enable_permute = FALSE;
			break;

			default:
				return ERROR_GETOPT;
			break;
		}

		option_pos++;
    }

    if (!options->mode)
    {
    	return ERROR_NOMODE;
    }
    else if (strlen(options->secret) == 0)
    {
    	return ERROR_NOSECRET;
    }
    else if (options->k == 0)
    {
    	return ERROR_NOK;
    }

    if (strlen(options->dir) == 0)
    {
    	strcpy(options->dir, DEFAULT_DIR);
    }

    return CMD_SUCCESS;
}

int validate_args(struct cmd_options *options)
{
	// el enunciado dice que -n solo se puede usar con -d pero da un ejemplo contradictorio
	if (options->n != 0 && options->mode == RECOVER_MODE)
	{
		printe("Error: n can only be specified when using the -d option.\n");
		return -1;
	}

	if (options->n != 0 && options->n < MIN_N)
	{
		printe("Error: n must be %d or greater.\n", MIN_N);
		return -1;
	}

	if (options->k < MIN_K)
	{
		printe("Error: k must be %d or greater.\n", MIN_K);
		return -1;
	}

	return 0;
}

void print_error(enum cmd_status status)
{
	switch (status)
	{
		case ERROR_D_AND_R:
			printe("Error: -r and -d cannot be defined simultaneously.\n");
		break;
		case ERROR_NOMODE:
			printe("Error: -r or -d must be defined.\n");
		break;
		case ERROR_NOK:
			printe("Error: K was invalid or was not specified.\n");
		break;
		case ERROR_NOSECRET:
			printe("Error: secret image was invalid or was not specified.\n");
		break;
		case ERROR_GETOPT:
			printe("Error: malformed argument detected.\n");
		break;
		case ERROR_NON:
			printe("Error: N was invalid or was not specified.\n");
		break;
		case ERROR_NODIR:
			printe("Error: directory was invalid or was not specified.\n");
		break;
		case ERROR_ORDER:
			printe("Error: arguments must be in the correct order.\n");
		break;
		default:
			printe("Unknown error.\n");
		break;
	}
}

void print_args_info(struct cmd_options *options)
{
	printv("Verbose mode enabled.\n");
	printv("Starting with:\n");
	printv("-> Secret file: \"%s\"\n", options->secret);
	printv("-> Mode: %s\n", options->mode == RECOVER_MODE ? "Recover" : "Distribute");
	printv("-> Directory: \"%s\"\n", options->dir);
	printv("-> Permutation: %s\n", options->enable_permute ? "Enabled" : "Disabled");
	printv("-> K: %d\n", options->k);
	if (options->n)
	{
		printv("-> N: %d\n", options->n);
	}
}

void print_bmps_info(struct bmp_handle **bmp_list, char **file_list, size_t len, int mode)
{
	int i;
	printf("Loaded images:\n");
	for (i = 0; i < len; i++)
	{
		struct bmp_header *header = bmp_get_header(bmp_list[i]);
		printf("-> %s [%ux%u] (Data Offset: 0x%x", file_list[i], header->width, header->height, header->offset);
		if (mode == RECOVER_MODE)
		{
			printf(", ShadowIndex: %u, ShadowSeed: %u)\n", header->shadow_index, header->seed);
		}
		else
		{
			printf(")\n");
		}
	}
}

int compare_strings(const void *a, const void *b)
{
	return strcmp(*(char**)a, *(char**)b);
}

int is_bmp_file(struct dirent *ep)
{
	char *filename = ep->d_name;
	size_t len = strlen(filename);
	if (len < 5 || ep->d_type != DT_REG)
	{
		return 0;
	}

	char *extension = &filename[len - 4];
	if (strcmp(extension, ".bmp") != 0)
	{
		return 0;
	}

	return 1;
}

char **bmps_in_dir(DIR *dp, int count, int *found)
{
	size_t bmp_count = 0;
	struct dirent *ep;

	if (dp == NULL || found == NULL)
	{
		return NULL;
	}

	while ((ep = readdir(dp)))
	{
		if (is_bmp_file(ep))
		{
			bmp_count++;
		}
	}

	if (bmp_count == 0 || (count != 0 && bmp_count < count))
	{
		return NULL;
	}

	rewinddir(dp);

	char **bmps = malloc(bmp_count * sizeof(char*));
	if (bmps == NULL)
	{
		return NULL;
	}

	int i = 0;
	while ((ep = readdir(dp)))
	{
		if (is_bmp_file(ep))
		{
			bmps[i++] = ep->d_name;
		}
	}

	qsort(bmps, bmp_count, sizeof(char*), compare_strings);
	*found = bmp_count;

	return bmps;
}

struct bmp_handle **open_files(char **file_list, int to_open, char *dir)
{
	int i;
	struct bmp_handle **bmp_list = malloc(to_open * sizeof(struct bmp_handle*));
	char tmp_filename[MAX_FILENAME_LEN] = {0};

	if (bmp_list == NULL)
	{
		return NULL;
	}

	for (i = 0; i < to_open; i++)
	{
		strcpy(tmp_filename, dir);
		strcat(tmp_filename, "/");
		strcat(tmp_filename, file_list[i]);

		bmp_list[i] = bmp_open(tmp_filename);
		if (bmp_list[i] == NULL)
		{
			bmp_free_list(bmp_list, i);
			return NULL;
		}
	}

	return bmp_list;
}

int check_bmp_sizes(struct bmp_handle **bmp_list, size_t len)
{
	if (len < 2)
	{
		return -1;
	}

	struct bmp_header *header = bmp_get_header(bmp_list[0]);
	bmp_dword_t first_width = header->width;
	bmp_dword_t first_height = header->height;

	int i;
	for (i = 1; i < len; i++)
	{
		header = bmp_get_header(bmp_list[i]);

		if (header->width != first_width || header->height != first_height)
		{
			return -1;
		}
	}

	return 0;
}

int check_shadow_sizes(struct bmp_handle *secret, struct bmp_handle **shadows, size_t len, int k)
{
	struct bmp_header *secret_header = bmp_get_header(secret);

	int padding = padding_for_width(secret_header->width);
	size_t real_byte_count = (secret_header->width + padding) * secret_header->height;

	size_t shadow_size = shadow_size_for(real_byte_count, k);
	int i;

	for (i = 0; i < len; i++)
	{
		struct bmp_header *header = bmp_get_header(shadows[i]);
		int shadow_padding = padding_for_width(header->width);
		size_t shadow_real_byte_count = (header->width + shadow_padding) * header->height;

		if (shadow_real_byte_count != shadow_size)
		{
			return -1;
		}

		if (k == 8)
		{
			if (header->width != secret_header->width || header->height != secret_header->height)
			{
				return -1;
			}
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct cmd_options options;
	enum cmd_status status = parse_args(argc, argv, &options);

	if (status != CMD_SUCCESS)
	{
		print_error(status);
		return EXIT_FAILURE;
	}

	if (validate_args(&options))
	{
		return EXIT_FAILURE;
	}

	printv("==== TP CRIPTO 2015 ====\n");
	print_args_info(&options);

	DIR *dp = opendir(options.dir);
	if (dp == NULL)
	{
		printe("Error: unable to open specified directory.\n");
		return EXIT_FAILURE;
	}

	char **file_list = NULL;
	int found = 0, to_open = 0;

	if (options.mode == RECOVER_MODE)
	{
		file_list = bmps_in_dir(dp, options.k, &found);
		if (file_list == NULL)
		{
			printe("Error: unable to open the required K = %d files.\n", options.k);
			goto free_dp;
		}

		if (found > options.k)
		{
			printv("WARNING: more than K = %d files were found, using K first files.\n", options.k);
		}

		to_open = options.k;
	}
	else // options.mode == DISTRIBUTE_MODE
	{
		file_list = bmps_in_dir(dp, options.n, &found);
		if (file_list == NULL)
		{
			printe("Error: unable to open the required files (bmps_in_dir).\n");
			goto free_dp;
		}

		if (options.n != 0 && found > options.n)
		{
			printv("WARNING: more than N = %d files were found, using N first files.\n", options.n);
		}

		if (!options.n)
		{
			options.n = found;
			printv("-> N: %d (total .bmp files found in directory)\n", options.n);
		}

		to_open = options.n;
	}

	if (options.mode == DISTRIBUTE_MODE && options.k > options.n)
	{
		printe("Error: K must be equal or less than N.\n");
		goto free_file_list;
	}

	struct bmp_handle **bmp_list = open_files(file_list, to_open, options.dir);
	if (bmp_list == NULL)
	{
		printe("Error: Unable to open the required files (open_files).\n");
		goto free_file_list;
	}

	if (check_bmp_sizes(bmp_list, to_open))
	{
		printe("Error: all images must have the same width and height.\n");
		goto free_bmp_list;
	}

	if (verbose_mode)
	{
		print_bmps_info(bmp_list, file_list, to_open, options.mode);
	}

	if (options.mode == RECOVER_MODE)
	{
		struct bmp_header *first_header = bmp_get_header(bmp_list[0]);
		struct bmp_handle *secret = NULL;

		printv("Secret image:\n");

		if (options.k == 8)
		{
			printv("-> Secret Width: %d\n", first_header->width);
			printv("-> Secret Height: %d\n", first_header->height);
			secret = bmp_create(options.secret, bmp_list[0], first_header->width, first_header->height);
		}
		else
		{
			if (options.secret_height == 0 || options.secret_width == 0)
			{
				printe("Error: invalid secret width/height specified (options -w and -h).\n");
				goto free_bmp_list;
			}

			printv("-> Secret Width: %d\n", options.secret_width);
			printv("-> Secret Height: %d\n", options.secret_height);
			secret = bmp_create(options.secret, bmp_list[0], options.secret_width, options.secret_height);
		}

		if (secret == NULL)
		{
			printe("Error: unable to create secret image.\n");
			goto free_bmp_list;
		}

		int status = recover(secret, bmp_list, options.k, options.enable_permute);
		if (status != 0)
		{
			printe("Error: An error occurred when recuperating the secret image.\n");
			goto free_bmp_list;
		}

		printv("Successfully recovered secret image to file: %s.\n", options.secret);
		bmp_free(secret);
	}
	else // options.mode == DISTRIBUTE_MODE
	{
		struct bmp_handle *secret = bmp_open(options.secret);
		if (secret == NULL)
		{
			printe("Error: unable to open target image \"%s\" to distribute.\n", options.secret);
			goto free_bmp_list;
		}

		printv("Opened secret image %s.\n", options.secret);

		if (check_shadow_sizes(secret, bmp_list, to_open, options.k))
		{
			printe("Error: one or more of the shadow images does not have the required size.\n");
			bmp_free(secret);
			goto free_bmp_list;
		}

		int status = distribute(secret, bmp_list, options.n, options.k, options.enable_permute);
		if (status != 0)
		{
			printe("Error: unable to distribute target image.\n");
			bmp_free(secret);
			goto free_bmp_list;
		}

		printv("Successfully distributed target image %s to (K=%d, N=%d) shadows.\n",options.secret, options.k, options.n);

		bmp_free(secret);
	}

	bmp_free_list(bmp_list, to_open);
	free(file_list);
	closedir(dp);

	return EXIT_SUCCESS;

// Error Handling

free_bmp_list:
	bmp_free_list(bmp_list, to_open);

free_file_list:
	free(file_list);
free_dp:
	closedir(dp);

	return EXIT_FAILURE;
}