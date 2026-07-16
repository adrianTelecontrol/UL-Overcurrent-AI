#ifndef BITMAP_PARSER_H_
#define BITMAP_PARSER_H_

#include <stdint.h>
#include <stdbool.h>

#include "fatfs/src/ff.h"

#define BPP_MONOCROME_PALETTE	1	//monochrome palette. NumColors = 1  
#define BPP_4BIT_PALLETIZED		4	//4bit palletized. NumColors = 16 
#define BPP_8BIT_PALLETIZED		8	//8bit palletized. NumColors = 256 
#define BPP_16BIT_RGB			16	//16bit RGB. NumColors = 65536
#define BPP_24BIT_RGB			24	//24bit RGB. NumColors = 16M

#define BITMAP_FILE_SIGNATURE   0x4D42 // BM literals of signature

typedef enum
 {
   BI_RGB = 0x0000,
   BI_RLE8 = 0x0001,
   BI_RLE4 = 0x0002,
   BI_BITFIELDS = 0x0003,
   BI_JPEG = 0x0004,
   BI_PNG = 0x0005,
   BI_CMYK = 0x000B,
   BI_CMYKRLE8 = 0x000C,
   BI_CMYKRLE4 = 0x000D
 } BMPCompression;

typedef struct {
	bool isValid;
	uint16_t bits_per_pixel;
	uint32_t compression_type;
	uint32_t size_info_header;
	uint32_t data_offset;
	uint8_t padding_bytes;
	uint16_t signature;	
	uint32_t file_size;	
	uint32_t bitmap_width;
	uint32_t bitmap_height;	
	uint32_t image_size_in_pixels;
	uint32_t image_size_in_bytes;
} BitmapHeader_t;

typedef struct
{
	uint8_t *ui8Pixels;
	uint16_t ui16Width;
	uint16_t ui16Height;

	BitmapHeader_t sHeader;
} BitmapHandler_t;

bool bitmap_Parser(FIL *psFileObject, BitmapHandler_t *psBitmapHandler);

void printBitmapHeader(BitmapHeader_t *psBitmapHeader);

#endif // BITMAP_PARSER_H_




