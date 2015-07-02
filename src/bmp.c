#include "bmp.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

#define BMP_SEED_OFFSET 6
#define BMP_SHADOW_INDEX_OFFSET 8
#define BMP_OFFSET_OFFSET 0xA
#define BMP_SIZE_OFFSET 0x22
#define BMP_TYPE 0x4D42

struct bmp_handle {
	FILE *file;
	bmp_byte_t *pixels;
	bmp_byte_t *extra_header;
	struct bmp_header header;
};

int bmp_valid_header(struct bmp_header *header);

struct bmp_handle *bmp_struct_init()
{
	struct bmp_handle *bmp = malloc(sizeof(struct bmp_handle));
	if (bmp == NULL)
	{
		return NULL;
	}

	bmp->extra_header = NULL;
	bmp->pixels = NULL;

	return bmp;
}

struct bmp_handle *bmp_open(const char *filename)
{
	if (filename == NULL)
	{
		return NULL;
	}

	struct bmp_handle *bmp = bmp_struct_init();

	if (bmp == NULL)
	{
		return NULL;
	}

	FILE *file = fopen(filename, "r+b");
	if (file == NULL)
	{
		goto free_bmp_handle;
	}

	bmp->file = file;

	// Read entire header
	if (fread(&bmp->header, sizeof(struct bmp_header), 1, file) != 1)
	{
		goto close_file;
	}

	struct bmp_header *header = &bmp->header;
	header->image_size = header->width * header->height;

	// Validate bmp header
	if (bmp_valid_header(&bmp->header) != 0)
	{
		goto close_file;
	}

	int padding = padding_for_width(header->width);
	size_t real_byte_count = (header->width + padding) * header->height;

	// Allocate space for image bytes
	bmp->pixels = malloc(real_byte_count * sizeof(bmp_byte_t));
	if (bmp->pixels == NULL)
	{
		goto close_file;
	}

	// Load image bytes to memmory
	if (fseek(bmp->file, (bmp->header).offset, 0) != 0)
	{
		goto free_pixels;
	}

	size_t read = fread(bmp->pixels, sizeof(bmp_byte_t), real_byte_count, file);
	if (read != real_byte_count)
	{
		goto free_pixels;
	}

	size_t extra_header_size = (bmp->header).offset - sizeof(struct bmp_header);

	bmp->extra_header = malloc((bmp->header).offset - sizeof(extra_header_size));
	if (bmp->extra_header == NULL)
	{
		goto free_pixels;
	}

	// Load the extra information
	if (fseek(bmp->file, sizeof(struct bmp_header), 0) != 0)
	{
		goto free_extra_header;
	}

	if (fread(bmp->extra_header, sizeof(bmp_byte_t), extra_header_size, file) != extra_header_size)
	{
		goto free_extra_header;
	}

	return bmp;

	// Error handling
free_extra_header:
	free(bmp->extra_header);
free_pixels:
	free(bmp->pixels);
close_file:
	fclose(file);
free_bmp_handle:
	free(bmp);

	return NULL;
}


int bmp_valid_header(struct bmp_header *header)
{
	// First 2 bytes must be 'BM'
	if (header->type != BMP_TYPE)
	{
		return -1;
	}

	// Must be 8 bits per pixel
	if (header->bits_per_pixel != 8)
	{
		return -1;
	}

	return 0;
}

struct bmp_handle* bmp_create(const char *filename, struct bmp_handle *bmp, bmp_dword_t width, bmp_dword_t height)
{
	if (filename == NULL || bmp == NULL)
	{
		return NULL;
	}

	struct bmp_handle *new_bmp = bmp_struct_init();
	if (new_bmp == NULL)
	{
		return NULL;
	}

	// If file exists, clear it. If it doesn't, create it.
	FILE *file = fopen(filename, "w+b");
	if (file == NULL)
	{
		goto free_bmp_handle;
	}

	new_bmp->file = file;
	// Copy header from another image
	if (memcpy(&new_bmp->header, &bmp->header, sizeof(struct bmp_header)) == NULL)
	{
		goto close_file;
	}


	int padding = padding_for_width(width);
	size_t real_byte_count = (width + padding) * height;

	(new_bmp->header).seed = 0;
	(new_bmp->header).shadow_index = 0;
	(new_bmp->header).image_size = width * height;
	(new_bmp->header).width = width;
	(new_bmp->header).height = height;
	(new_bmp->header).size = 0;

	if (bmp_write_header(new_bmp) != 0)
	{
		goto close_file;
	}

	if (fseek(new_bmp->file, sizeof(struct bmp_header), 0) != 0)
	{
		goto close_file;
	}

	size_t extra_header_size = (new_bmp->header).offset - sizeof(struct bmp_header);
	new_bmp->extra_header = malloc((bmp->header).offset - sizeof(extra_header_size));
	memcpy(new_bmp->extra_header, bmp->extra_header, extra_header_size);
	if (fwrite(new_bmp->extra_header, sizeof(bmp_byte_t), extra_header_size, file) != extra_header_size)
	{
		goto close_file;
	}

	// Allocate space for image bytes
	new_bmp->pixels = calloc(real_byte_count, sizeof(bmp_byte_t));
	if (new_bmp->pixels == NULL)
	{
		goto close_file;
	}

	return new_bmp;

	// Error handling

close_file:
	fclose(file);
free_bmp_handle:
	free(bmp);

	return NULL;

}

void bmp_free(struct bmp_handle *bmp)
{
	if (bmp == NULL)
	{
		return;
	}

	fclose(bmp->file);
	free(bmp->pixels);
	free(bmp->extra_header);
	free(bmp);
}

void bmp_free_list(struct bmp_handle **bmp_list, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
	{
		bmp_free(bmp_list[i]);
	}
}

int bmp_write_pixels(struct bmp_handle *bmp)
{
	if (bmp == NULL || bmp->pixels == NULL)
	{
		return -1;
	}

	if (fseek(bmp->file, (bmp->header).offset, 0) != 0)
	{
		return -1;
	}

	int padding = padding_for_width((bmp->header).width);
	size_t real_byte_count = ((bmp->header).width + padding) * (bmp->header).height;

	if (fwrite(bmp->pixels, sizeof(bmp_byte_t), real_byte_count, bmp->file) != real_byte_count)
	{
		return -1;
	}

	return 0;
}

int bmp_write_header(struct bmp_handle *bmp)
{
	if (fseek(bmp->file, 0, 0) != 0)
	{
		return -1;
	}

	if (fwrite(&bmp->header, sizeof(struct bmp_header), 1, bmp->file) != 1)
	{
		return -1;
	}

	return 0;
}

// Getters

struct bmp_header *bmp_get_header(struct bmp_handle *bmp)
{
	if (bmp == NULL)
	{
		return NULL;
	}
	return &bmp->header;
}

bmp_byte_t *bmp_get_pixels(struct bmp_handle *bmp)
{
	if (bmp == NULL)
	{
		return NULL;
	}

	return bmp->pixels;
}
