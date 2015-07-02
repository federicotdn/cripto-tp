#ifndef BMP_H
#define BMP_H

#include <stdint.h>
#include <stdlib.h>

typedef uint32_t bmp_dword_t;
typedef uint16_t bmp_word_t;
typedef unsigned char bmp_byte_t;
typedef int bmp_op_t;
typedef uint16_t bmp_seed_t;
typedef uint16_t bmp_shadow_index_t;

#pragma pack(push, 1)

struct bmp_header {
	bmp_word_t type; // file type
	bmp_dword_t size; // file size
	bmp_word_t seed; // reserved. Used for seed.
	bmp_word_t shadow_index; // reserved. Used to save the shadow index.
	bmp_dword_t offset; // offset in bytes to image data
	bmp_dword_t info_header_size; // size of the header (40 bytes)
	bmp_dword_t width; // width of the image in pixels
	bmp_dword_t height; // width of the image in pixels
	bmp_word_t color_planes; // number of planes, must be 1
	bmp_word_t bits_per_pixel; // number of bits per pixel
	bmp_dword_t compression_method; // compression method used
	bmp_dword_t image_size; // image size
	bmp_dword_t horizontal_resolution; // horizontal resolution
	bmp_dword_t vertical_resolution; // horizontal resolution
	bmp_dword_t number_of_colors; // number of colors in palette
	bmp_dword_t number_of_i_colors; // number of important colors
};

#pragma pack(pop)

struct bmp_handle;
struct bmp_handle *bmp_open(const char *filename);
void bmp_free(struct bmp_handle *bmp);
void bmp_free_list(struct bmp_handle **bmp_list, size_t len);
int bmp_write_pixels(struct bmp_handle *bmp);
int bmp_write_header(struct bmp_handle *bmp);
struct bmp_handle* bmp_create(const char *filename, struct bmp_handle *bmp, bmp_dword_t width, bmp_dword_t height);

// Getters
struct bmp_header *bmp_get_header(struct bmp_handle *bmp);
bmp_byte_t *bmp_get_pixels(struct bmp_handle *bmp);

#endif
/* BMP_H */