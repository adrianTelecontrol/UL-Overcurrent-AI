/**
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <fatfs/src/ff.h>
#include <fatfs/src/integer.h>

#include "helpers.h"

#include "bitmap_parser.h"

static const char TASK_NAME[] = "BITMAP_HANDLER";

bool bitmap_Parser(FIL *psFileObject, BitmapHandler_t *psBitmapHandler) {
  UINT ui32BytesRead; // CRITICAL: Required by f_read to avoid Tiva Hard Fault

  // 1. Get File Size
  psBitmapHandler->sHeader.file_size = f_size(psFileObject);

  // 2. Read Signature (2 Bytes)
  f_lseek(psFileObject, 0x00);
  f_read(psFileObject, &psBitmapHandler->sHeader.signature, 2, &ui32BytesRead);

  // 3. Read Data Offset (4 Bytes)
  f_lseek(psFileObject, 0x0A);
  f_read(psFileObject, &psBitmapHandler->sHeader.data_offset, 4,
         &ui32BytesRead);

  // 4. Read Size Info Header (4 Bytes)
  f_lseek(psFileObject, 0x0E);
  f_read(psFileObject, &psBitmapHandler->sHeader.size_info_header, 4,
         &ui32BytesRead);

  // 5. Read Width (4 Bytes)
  f_lseek(psFileObject, 0x12);
  f_read(psFileObject, &psBitmapHandler->sHeader.bitmap_width, 4,
         &ui32BytesRead);

  // 6. Read Height (4 Bytes)
  f_lseek(psFileObject, 0x16);
  f_read(psFileObject, &psBitmapHandler->sHeader.bitmap_height, 4,
         &ui32BytesRead);

  // 7. Read Bits Per Pixel (2 Bytes)
  f_lseek(psFileObject, 0x1C);
  f_read(psFileObject, &psBitmapHandler->sHeader.bits_per_pixel, 2,
         &ui32BytesRead);

  // 8. Read Compression Type (4 Bytes)
  f_lseek(psFileObject, 0x1E);
  f_read(psFileObject, &psBitmapHandler->sHeader.compression_type, 4,
         &ui32BytesRead);

  // --- Logic & Calculations ---

  // Use absolute values (Height is negative in Top-Down BMPs)
  uint32_t absWidth = abs(psBitmapHandler->sHeader.bitmap_width);
  uint32_t absHeight = abs(psBitmapHandler->sHeader.bitmap_height);

  // Calculate Data Size
  // Note: Trust the file size - offset rather than the header image size field
  // (which is often 0 in uncompressed BI_RGB images)
  psBitmapHandler->sHeader.image_size_in_bytes =
      psBitmapHandler->sHeader.file_size - psBitmapHandler->sHeader.data_offset;

  psBitmapHandler->sHeader.image_size_in_pixels = absWidth * absHeight;

  // Calculate Padding (Standard BMP Formula)
  // 1. Calculate the 'Stride' (Row size aligned to 4 bytes)
  //    Formula: ((Width * BPP + 31) / 32) * 4
  uint32_t stride =
      ((absWidth * psBitmapHandler->sHeader.bits_per_pixel + 31) / 32) * 4;

  // 2. Calculate actual data bytes per row
  uint32_t dataBytesPerRow =
      (absWidth * psBitmapHandler->sHeader.bits_per_pixel) / 8;

  // 3. Padding is the difference
  psBitmapHandler->sHeader.padding_bytes = stride - dataBytesPerRow;

  // --- Reset Pointers ---
  psBitmapHandler->sHeader.isValid = true;

  // --- Validation Checks ---

  // 1. Check Signature 'BM' (0x4D42)
  //    Note: Little Endian 'BM' is 0x4D42. If read as uint16, it is 0x4D42.
  if (psBitmapHandler->sHeader.signature != 0x4D42) {
    psBitmapHandler->sHeader.isValid = false;
  }

  // 2. Check BPP (Only supporting 16-bit)
  if (psBitmapHandler->sHeader.bits_per_pixel != 16) {
    psBitmapHandler->sHeader.isValid = false;
  }

  // 3. Check Compression (BI_RGB=0 or BI_BITFIELDS=3)
  if ((psBitmapHandler->sHeader.compression_type != BI_RGB) && // BI_RGB
      (psBitmapHandler->sHeader.compression_type !=
       BI_BITFIELDS)) // BI_BITFIELDS
  {
    psBitmapHandler->sHeader.isValid = false;
  }

  // 4. Check Dimensions (Optional: Against your screen limits)
  // if (absWidth > 800 || absHeight > 480) psBitmapHandler->sHeader.isValid =
  // false;

  return psBitmapHandler->sHeader.isValid;
}

void printBitmapHeader(BitmapHeader_t *psBitmapHeader) {
  TIVA_LOGI(TASK_NAME, "-----------------------------");
  TIVA_LOGI(TASK_NAME, "------- BITMAP HEADER -------");
  TIVA_LOGI(TASK_NAME, "Signature: \t\t0x%4x", psBitmapHeader->signature);
  TIVA_LOGI(TASK_NAME, "Data Offset: \t\t0x%x", psBitmapHeader->data_offset);
  TIVA_LOGI(TASK_NAME, "Size info header: \t\t0x%6x",
            psBitmapHeader->size_info_header);
  TIVA_LOGI(TASK_NAME, "Size (w, h): \t\t(%u, %u)",
            psBitmapHeader->bitmap_width, psBitmapHeader->bitmap_height);
  TIVA_LOGI(TASK_NAME, "Bits per pixel: \t%u pixels",
            psBitmapHeader->bits_per_pixel);
  TIVA_LOGI(TASK_NAME, "Compression type: \t0x%4x",
            psBitmapHeader->compression_type);
  TIVA_LOGI(TASK_NAME, "Bitmap size: %u bytes",
            psBitmapHeader->image_size_in_bytes - psBitmapHeader->size_info_header);
  TIVA_LOGI(TASK_NAME, "-----------------------------");
}
