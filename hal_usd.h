#ifndef SDSPI_HAL_H_
#define SDSPI_HAL_H_

#include <stdint.h>
#include <stdbool.h>

#include <fatfs/src/ff.h>

#include "font_engine.h"
#include "bitmap_parser.h"
#include "render.h"

bool HAL_uSD_init(void);

#endif // SDSPI_HAL_H_

