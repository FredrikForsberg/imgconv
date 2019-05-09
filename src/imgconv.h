#pragma once
#include <stdlib.h>  // atoi
#include <stdio.h>   // printf
#include <string.h>
#include <limits.h>
#include <inttypes.h>

#include <png.h>
#include <jpeglib.h>
#include <stdlib.h>
#include "build.h"

#define CODEC_INVALID 0
#define CODEC_AUTO    1
// File formats
#define CODEC_JPEG    2
#define CODEC_PNG     3
// Raw formats
#define CODEC_RGBA    4
#define CODEC_RGB     5

/* TODO
#define CODEC_RGBA16  0 // 16 bits per channel, aka 2 bytes
#define CODEC_RGB16   0
#define CODEC_GA      0 // Grey + alpha
#define CODEC_GA16    0
#define CODEC_G       0 // Grey
#define CODEC_G16     0
*/

// Return codes
#define IMGCONV_SUCCESS               0
#define IMGCONV_ERROR_GENERIC         1
#define IMGCONV_ERROR_FILE            2
#define IMGCONV_ERROR_FORMAT          3
#define IMGCONV_ERROR_NOT_IMPLEMENTED 255

// Program modes
#define MODE_INVALID   0
#define MODE_INFO      1
#define MODE_TRANSCODE 2

// Struct to hold raw image data
struct imgconv_data {
	unsigned int  width, height, format, stride;
	void          *buffer;
	unsigned char bpc;
};

// Convert codec name to string
const char *imgconv_get_codec_name(int codec) {
	switch (codec) {
	case CODEC_JPEG:
		return "JPEG";
	case CODEC_PNG:
		return "PNG";
	case CODEC_RGB:
		return "RGB";
	case CODEC_RGBA:
		return "RGBA";
	}
	return "";
}

// Convert error code to string
char *imgconv_get_error(int code) {
	switch (code) {
	case IMGCONV_ERROR_FILE:
		return "file i/o error";
	case IMGCONV_ERROR_GENERIC:
		return "internal error";
	case IMGCONV_ERROR_FORMAT:
		return "image format not supported";
	case IMGCONV_ERROR_NOT_IMPLEMENTED:
		return "not implemented";
	}
	return "";
}

int imgconv_premultiply_alpha(struct imgconv_data *img_in, struct imgconv_data *img_out) {
	// Only handle 8bit/channel structs for now
	if (img_in->format != CODEC_RGBA || img_in->bpc != 8) return IMGCONV_ERROR_FORMAT;
	uint32_t n_pixels  = img_in->width * img_in->height;
	uint32_t max_value = 255;
	for (uint32_t i=0; i<n_pixels; i++) {
		int r,g,b,a;
		r = ((uint8_t*)img_in->buffer)[i*img_in->stride+0];
		g = ((uint8_t*)img_in->buffer)[i*img_in->stride+1];
		b = ((uint8_t*)img_in->buffer)[i*img_in->stride+2];
		a = ((uint8_t*)img_in->buffer)[i*img_in->stride+3];
		((uint8_t*)img_out->buffer)[i*img_out->stride+0] = r * a / max_value;
		((uint8_t*)img_out->buffer)[i*img_out->stride+1] = g * a / max_value;
		((uint8_t*)img_out->buffer)[i*img_out->stride+2] = b * a / max_value;
	}
	return IMGCONV_SUCCESS;
}

int imgconv_convert_data(struct imgconv_data *img_in, struct imgconv_data *img_out, int format) {
	// Copy all to out struct
	*img_out = *img_in;
	img_out->format = format;
	// Check if we already are in correct format
	if (img_in->format == format) {
		return IMGCONV_SUCCESS;
	}
	// RGBA -> RGB
	if (img_in->format == CODEC_RGBA && format == CODEC_RGB) {
		img_out->stride = 3;
		img_out->bpc    = 8;
		size_t n_pixels = img_out->width * img_out->height;
		img_out->buffer = malloc(n_pixels * img_out->stride);
		return imgconv_premultiply_alpha(img_in, img_out);
	}
	// Can't convert image
	return IMGCONV_ERROR_FORMAT;
}

int imgconv_write_png(FILE *fp, struct imgconv_data *img) {
	int         err        = 0;
	png_structp png_ptr    = NULL;
	png_infop   info_ptr   = NULL;
	int         color_type = 0;
	int         bit_depth  = img->bpc;
	// Check if data is correct
	if (img->format == CODEC_RGBA && img->stride == 4) {
		color_type = PNG_COLOR_TYPE_RGB_ALPHA;
	} else if (img->format == CODEC_RGB && img->stride == 3) {
		color_type = PNG_COLOR_TYPE_RGB;
	} else {
		// TODO: convert?
		return IMGCONV_ERROR_FORMAT;
	}

	// Corrupt data?
	if (bit_depth != 8 && bit_depth != 16) {
		return IMGCONV_ERROR_FORMAT;
	}

	if (fp == NULL) {
		err = IMGCONV_ERROR_FILE;
		goto cleanup;
	}
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		err = IMGCONV_ERROR_GENERIC;
		goto cleanup;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (png_ptr == NULL) {
        err = IMGCONV_ERROR_GENERIC;
        goto cleanup;
    }
	if (setjmp(png_jmpbuf(png_ptr))) {
		err = IMGCONV_ERROR_GENERIC;
		goto cleanup;
	}
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, img->width, img->height,
				 bit_depth, color_type, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(png_ptr, info_ptr);
	for (int y=0; y<img->height; y++) {
		unsigned int index = (img->width * y) * img->stride;
		png_bytep ptr = (png_bytep)&(((uint8_t*)img->buffer)[index]);
		png_write_row(png_ptr, ptr);
	}
	png_write_end(png_ptr, NULL);
 cleanup:
	if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL || info_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	return err;
}

int imgconv_read_png(FILE *fp, struct imgconv_data *img) {
	if ( ! fp) return 1;
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if ( ! png) return 1;
	png_infop info = png_create_info_struct(png);
	if ( ! info) return 1;
	if (setjmp(png_jmpbuf(png))) return 1;
	png_init_io(png, fp);
	// Get image info
	png_read_info(png, info);
	int width      = png_get_image_width(png, info);
	int height     = png_get_image_height(png, info);
	int color_type = png_get_color_type(png, info);
	int bit_depth  = png_get_bit_depth(png, info);
	// Convert image to 8 bit
	if (bit_depth == 16) {
		png_set_strip_16(png);
	}
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png);
	}
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
		png_set_expand_gray_1_2_4_to_8(png);
	}
	// Add alpha channel
	if (png_get_valid(png, info, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(png);
	}
	if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
	}
	// Convert gray to rgb
	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png);
	}
	color_type = PNG_COLOR_TYPE_RGB_ALPHA;
	bit_depth  = 8;
	png_read_update_info(png, info);
	// Allocate memory and read image data
	png_bytep buffer = (png_bytep)malloc(width * height * 4); // 4 = RGBA
	for (int y=0; y<height; y++) {
		png_read_row(png, (png_bytep)&(buffer[y*width*4]), NULL);
	}
	img->buffer = buffer;
	img->width  = width;
	img->height = height;
	img->bpc    = 8;
	img->stride = 4;
	img->format = CODEC_RGBA;
	return 0;
}

int imgconv_read_jpeg(FILE *fp, struct imgconv_data *img_in) {
	return IMGCONV_ERROR_NOT_IMPLEMENTED;
}

int imgconv_write_jpeg(FILE *fp, struct imgconv_data *img_in, int quality) {
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];
	int row_stride;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, fp);

	// Convert image to RGB
	struct imgconv_data img;
	int err = imgconv_convert_data(img_in, &img, CODEC_RGB);
	if (err != IMGCONV_SUCCESS) return err;

	// Set output parameters and defaults
	cinfo.image_width      = img.width;
	cinfo.image_height     = img.height;
	cinfo.input_components = 3; // 1
	cinfo.in_color_space   = JCS_RGB; // JCS_GRAYSCALE
	jpeg_set_defaults(&cinfo);
	if (quality >= 90) {
		cinfo.comp_info[0].h_samp_factor = 1;
		cinfo.comp_info[0].v_samp_factor = 1;
		cinfo.comp_info[1].h_samp_factor = 1;
		cinfo.comp_info[1].v_samp_factor = 1;
		cinfo.comp_info[2].h_samp_factor = 1;
		cinfo.comp_info[2].v_samp_factor = 1;
	}
	cinfo.dct_method       = JDCT_FLOAT;
	cinfo.optimize_coding  = TRUE; // More RAM+cpu, but lesser output file size
	jpeg_set_quality(&cinfo, quality, TRUE); // TRUE for force baseline
	jpeg_start_compress(&cinfo, TRUE);
	row_stride = cinfo.image_width * cinfo.input_components;
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &(((uint8_t*)img.buffer)[cinfo.next_scanline * row_stride]);
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	if (img.buffer != img_in->buffer) {
		free(img.buffer);
	}
	return IMGCONV_SUCCESS;
}

void parseSizeArgument(const char *input, int *width, int *height) {
	int i=0;
	while (input[i] != '\0' && input[i] != 'x' && input[i] != 'X') {
		i++;
	}
	*width = atoi(input);
	*height = atoi(&input[i+1]);
}

void defaultValues(int *width, int *height, int *type, int *quality) {
	*width   = 0;
	*height  = 0;
	*type    = CODEC_AUTO;
	*quality = 92;
}

int execute_arguments(int argc, const char **argv) {
	int mode    = MODE_TRANSCODE;

	// temp values
	int width, height, type, quality;
	defaultValues(&width, &height, &type, &quality);

	// File in config
	const char *in_file  = NULL;
	int in_width, in_height, in_codec;

	// File out config
	const char *out_file = NULL;
	int out_codec, out_quality;

	// Parse arguments
	for (int i=1; i<argc; i++) {
		const char *arg = argv[i];
		if (strcmp(arg,"-version") == 0) {
			fprintf(stderr,"Build date: %s\nRevision: %s\n", IMGCONV_BUILD_DATE, IMGCONV_BUILD_REV);
			exit(0);
		} else if (strcmp(arg,"-info") == 0) {
			mode = MODE_INFO;
		} else if (strcmp(arg,"-dimension") == 0) {
			parseSizeArgument(argv[++i], &width, &height);
		} else if (strcmp(arg,"-codec") == 0) {
			arg = argv[++i];
			if (strcmp(arg,"JPEG") == 0) {
				type = CODEC_JPEG;
			} else if (strcmp(arg,"PNG") == 0) {
				type = CODEC_PNG;
			} else if (strcmp(arg,"RGBA") == 0) {
				type = CODEC_RGBA;
			} else if (strcmp(arg,"RGB") == 0) {
				type = CODEC_RGB;
			} else {
				fprintf(stderr, "Unknown type '%s' see help for recognized codecs\n",arg);
				exit(1);
			}
		} else if (strcmp(arg,"-quality") == 0) {
			arg      = argv[++i];
			quality  = atoi(arg);
		} else if (strcmp(arg,"-input") == 0) {
			in_file   = argv[++i];
			in_width  = width;
			in_height = height;
			in_codec  = type;
			defaultValues(&width, &height, &type, &quality);
		} else if (strcmp(arg,"-output") == 0) {
			out_file    = argv[++i];
			out_codec   = type;
			out_quality = quality;
			defaultValues(&width, &height, &type, &quality);
		} else {
			fprintf(stderr, "Unknown argument '%s' see help for recognized arguments\n",arg);
		}
	}

	// Valid input/output?
	if (in_file == NULL) {
		fprintf(stderr, "Missing '-input' argument\n");
		exit(1);
	}
	if (out_file == NULL && mode == MODE_TRANSCODE) {
		fprintf(stderr, "Missing '-output' argument\n");
		exit(1);
	}

	// Blob data need to specify dimension
	if (in_codec == CODEC_RGBA) {
		if (in_width <= 0 || in_height <= 0) {
			fprintf(stderr, "Missing '-dimension' before '-input' with width and height bigger than 0\n");
			exit(1);
		}
	}

	// Read input file
	FILE *in_fd;
	if (strcmp(in_file, "-") == 0) {
		in_fd = stdin;
	} else {
		in_fd = fopen(in_file, "rb");
	}
	struct imgconv_data in_data;
	if (in_codec == CODEC_RGBA || in_codec == CODEC_RGB) {
		if (in_codec == CODEC_RGB) {
			in_data.stride = 3;
		} else if (in_codec == CODEC_RGBA) {
			in_data.stride = 4;
		}
		uint8_t *in_buffer = (uint8_t*)malloc(in_width * in_height * in_data.stride);
		fread(in_buffer, in_data.stride, in_width * in_height, in_fd);
		in_data.buffer = in_buffer;
		in_data.format = in_codec;
		in_data.bpc    = 8;
		in_data.height = in_height;
		in_data.width  = in_width;
	} else if (in_codec == CODEC_PNG) {
		if (imgconv_read_png(in_fd, &in_data) != IMGCONV_SUCCESS) {
			fprintf(stderr, "Reading png failed\n");
			exit(1);
		}
	} else if (in_codec == CODEC_JPEG) {
		int err = imgconv_read_jpeg(in_fd, &in_data);
		if (err != IMGCONV_SUCCESS) {
			fprintf(stderr, "Reading jpeg failed due to %s\n", imgconv_get_error(err));
			exit(1);
		}
	} else {
		fprintf(stderr, "Unknown input file format\n");
		exit(1);
	}
	fclose(in_fd);

	
	// Write output file
	FILE *out_fd;
	if (out_file == NULL || strcmp(out_file, "-") == 0) {
		out_fd = stdout;
	} else {
		out_fd = fopen(out_file, "wb");
	}
	if (mode == MODE_INFO) {
		fprintf(out_fd, "{\"width\": %d, \"height\": %d, \"bpc\": %d, \"stride\": %d, \"codec\": \"%s\"}\n",
				in_data.width, in_data.height, in_data.bpc, in_data.stride, imgconv_get_codec_name(in_data.format));
	} else if (out_codec == CODEC_PNG) {
		int err;
		if ((err = imgconv_write_png(out_fd, &in_data)) != IMGCONV_SUCCESS) {
			fprintf(stderr, "Writing png failed due to %s\nUsing codec %d, bpc %d, stride %d\n",imgconv_get_error(err), in_data.format, in_data.bpc, in_data.stride);
			exit(1);
		}
	} else if (out_codec == CODEC_RGBA || out_codec == CODEC_RGB) {
		struct imgconv_data out_data;
		int err = imgconv_convert_data(&in_data, &out_data, out_codec);
		if (err != IMGCONV_SUCCESS) {
			fprintf(stderr, "Can't convert data\n");
			exit(1);
		}
		if (out_data.width*out_data.height != fwrite(out_data.buffer, out_data.stride, out_data.width * out_data.height, out_fd)) {
			fprintf(stderr, "Writing to file failed\n");
			exit(1);
		}
		if (out_data.buffer != in_data.buffer) {
			free(out_data.buffer);
		}
	} else if (out_codec == CODEC_JPEG) {
		int err = imgconv_write_jpeg(out_fd, &in_data, out_quality);
		if (err != IMGCONV_SUCCESS) {
			fprintf(stderr, "Writing jpeg failed due to %s\n", imgconv_get_error(err));
			exit(1);
		}
	} else {
		fprintf(stderr, "Unknown output file format\n");
		exit(1);
	}
	fclose(out_fd);

	// All done!
	return 0;
}
