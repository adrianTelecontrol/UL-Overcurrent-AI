
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tiva_log.h"
#include "helpers.h"

#include "file_manager.h"

#define PATH_BUF_SIZE 100
static const char TASK_NAME[] = "FILE_MANAGER";

static FM_EVEImageAsset_t g_sEVEImageAssets[FM_EVE_IMAGE_MAX_ASSETS];
static uint8_t g_ui8EVEImageAssetCount = 0;
static uint32_t g_ui32NextEVEImageRAMG = FM_EVE_RAMG_ASSET_START;

// Estructura para mapear códigos de error a texto
typedef struct {
    FRESULT iFResult;
    const char *pcResultStr;
} tFResultString;

static const tFResultString g_sFResultStrings[] = {
    {FR_OK, "FR_OK"}, {FR_DISK_ERR, "FR_DISK_ERR"}, {FR_INT_ERR, "FR_INT_ERR"},
    {FR_NOT_READY, "FR_NOT_READY"}, {FR_NO_FILE, "FR_NO_FILE"}, {FR_NO_PATH, "FR_NO_PATH"},
    {FR_INVALID_NAME, "FR_INVALID_NAME"}, {FR_DENIED, "FR_DENIED"}, {FR_EXIST, "FR_EXIST"},
    {FR_INVALID_OBJECT, "FR_INVALID_OBJECT"}, {FR_WRITE_PROTECTED, "FR_WRITE_PROTECTED"},
    {FR_INVALID_DRIVE, "FR_INVALID_DRIVE"}, {FR_NOT_ENABLED, "FR_NOT_ENABLED"},
    {FR_NO_FILESYSTEM, "FR_NO_FILESYSTEM"}, {FR_MKFS_ABORTED, "FR_MKFS_ABORTED"},
    {FR_TIMEOUT, "FR_TIMEOUT"}, {FR_LOCKED, "FR_LOCKED"}, {FR_NOT_ENOUGH_CORE, "FR_NOT_ENOUGH_CORE"},
    {FR_TOO_MANY_OPEN_FILES, "FR_TOO_MANY_OPEN_FILES"}, {FR_INVALID_PARAMETER, "FR_INVALID_PARAMETER"}
};

static uint32_t FM_AlignRAMGAddress(uint32_t address) {
    return (address + (FM_EVE_RAMG_ALIGNMENT - 1U)) &
           ~(FM_EVE_RAMG_ALIGNMENT - 1U);
}

static void FM_ApplyEVEImageAssetToWidget(const FM_EVEImageAsset_t *asset,
                                          gfx_Image *img) {
    if (asset == NULL || img == NULL) {
        return;
    }

    img->ramgAddress = asset->ramgAddress;
    img->ramgSizeBytes = asset->ramgSizeBytes;
    img->fileSizeBytes = asset->fileSizeBytes;
    img->size.width = asset->width;
    img->size.height = asset->height;

    if (img->scale == 0U) {
        img->scale = 1U;
    }
}

void FM_EVEImageRegistryReset(uint32_t ramgStartAddress) {
    uint8_t i = 0;

    for (; i < FM_EVE_IMAGE_MAX_ASSETS; i++) {
        memset(&g_sEVEImageAssets[i], 0, sizeof(g_sEVEImageAssets[i]));
    }

    g_ui8EVEImageAssetCount = 0;
    g_ui32NextEVEImageRAMG =
        FM_AlignRAMGAddress(ramgStartAddress == 0U
                                ? FM_EVE_RAMG_ASSET_START
                                : ramgStartAddress);
}

const FM_EVEImageAsset_t *FM_EVEImageFind(const uint8_t drive,
                                          const char *pcFilePath) {
    uint8_t i = 0;

    if (pcFilePath == NULL) {
        return NULL;
    }

    for (; i < g_ui8EVEImageAssetCount; i++) {
        if (g_sEVEImageAssets[i].bIsValid &&
            g_sEVEImageAssets[i].drive == drive &&
            strcmp(g_sEVEImageAssets[i].path, pcFilePath) == 0) {
            return &g_sEVEImageAssets[i];
        }
    }

    return NULL;
}

const FM_EVEImageAsset_t *FM_EVEImageGet(uint8_t index) {
    if (index >= g_ui8EVEImageAssetCount) {
        return NULL;
    }

    return &g_sEVEImageAssets[index];
}

uint8_t FM_EVEImageGetCount(void) {
    return g_ui8EVEImageAssetCount;
}

uint32_t FM_EVEImageGetNextRAMGAddress(void) {
    return g_ui32NextEVEImageRAMG;
}

static bool FM_RegisterEVEImageAsset(const uint8_t drive,
                                     const char *pcFilePath,
                                     const gfx_Image *img) {
    FM_EVEImageAsset_t *asset;
    uint32_t ramgEnd;

    if (pcFilePath == NULL || img == NULL || img->ramgSizeBytes == 0U) {
        return false;
    }

    if (g_ui8EVEImageAssetCount >= FM_EVE_IMAGE_MAX_ASSETS) {
        TIVA_LOGE(TASK_NAME, "EVE image registry is full");
        return false;
    }

    ramgEnd = FM_AlignRAMGAddress(img->ramgAddress + img->ramgSizeBytes);
    if (ramgEnd > RAM_G_SIZE) {
        TIVA_LOGE(TASK_NAME, "EVE image exceeds RAM_G: end=%lu limit=%lu",
                  ramgEnd, (uint32_t)RAM_G_SIZE);
        return false;
    }

    asset = &g_sEVEImageAssets[g_ui8EVEImageAssetCount];
    memset(asset, 0, sizeof(*asset));

    asset->bIsValid = true;
    asset->drive = drive;
    strncpy(asset->path, pcFilePath, FM_EVE_IMAGE_PATH_MAX - 1U);
    asset->path[FM_EVE_IMAGE_PATH_MAX - 1U] = '\0';
    asset->ramgAddress = img->ramgAddress;
    asset->ramgSizeBytes = img->ramgSizeBytes;
    asset->fileSizeBytes = img->fileSizeBytes;
    asset->width = img->size.width;
    asset->height = img->size.height;

    g_ui8EVEImageAssetCount++;
    if (ramgEnd > g_ui32NextEVEImageRAMG) {
        g_ui32NextEVEImageRAMG = ramgEnd;
    }

    return true;
}

const char* FM_StringFromFResult(FRESULT iFResult) {
    uint8_t ui8Idx;
    for (ui8Idx = 0; ui8Idx < (sizeof(g_sFResultStrings) / sizeof(tFResultString)); ui8Idx++) {
        if (g_sFResultStrings[ui8Idx].iFResult == iFResult) {
            return g_sFResultStrings[ui8Idx].pcResultStr;
        }
    }
    return "UNKNOWN_ERR";
}

const char* FM_getDriveString(uint8_t drive) {
	switch (drive) {
		case DRIVE_SD_ID: return "0:";
		case DRIVE_USB_ID: return "1:";
		default: return NULL;
	}
}

int FM_FetchFile(const char *drive, const char *pcFilePath, uint8_t *pui32SDRAMBuff, uint32_t ui32BuffSize) {
    FRESULT iFResult;
    FIL sFileObject; // Quitado el 'static' (es más seguro dejarlo en el stack para re-entrada)
    UINT ui32BytesRead = 0;

    // Validación de seguridad
    if(pcFilePath == NULL || pui32SDRAMBuff == NULL || ui32BuffSize == 0 || drive == NULL ) {
        return -1; 
    }
	
	char tempFilePath[20];	

	snprintf(tempFilePath, sizeof(tempFilePath), "%s/%s", drive, pcFilePath);

    TIVA_LOGI(TASK_NAME, "File to load: %s", tempFilePath);

    // 1. Abrir el archivo
    iFResult = f_open(&sFileObject, tempFilePath, FA_READ);
    if (iFResult != FR_OK) {
        TIVA_LOGE(TASK_NAME, "Error opening file: %d", iFResult);
        return -1;
    }

    // 2. Comprobar que el archivo cabe en la porción de SDRAM reservada
    uint32_t fileSize = f_size(&sFileObject);
    if (fileSize > ui32BuffSize) {
        TIVA_LOGE(TASK_NAME, "Error: File size (%lu) exceeds buffer size (%lu)", fileSize, ui32BuffSize);
        f_close(&sFileObject); // ¡Crucial cerrar antes de salir!
        return -1;
    }

    // 3. Lectura directa a la SDRAM (Máximo rendimiento, sin buffers intermedios)
    iFResult = f_read(&sFileObject, pui32SDRAMBuff, fileSize, &ui32BytesRead);

    // 4. Cerrar el archivo (SIEMPRE debe ejecutarse)
    f_close(&sFileObject);

    // 5. Validar la lectura
    if (iFResult != FR_OK) {
        TIVA_LOGE(TASK_NAME, "Error reading file: %d", iFResult);
        return -1;
    }

    if (ui32BytesRead != fileSize) {
        TIVA_LOGE(TASK_NAME, "Warning: Read bytes (%u) mismatch file size (%lu)", ui32BytesRead, fileSize);
        // Dependiendo de tu lógica, esto podría ser un error o no.
    }

    TIVA_LOGI(TASK_NAME, "File copy complete! Bytes: %u", ui32BytesRead);

    // Retornamos la cantidad de bytes reales que se copiaron a la SDRAM
    return (int)ui32BytesRead;
}

int FM_FetchBitmap(const char *drive, const char *pcFilePath, BitmapHandler_t *psBitmapHandler, const uint32_t ui32BuffSize) {
    FRESULT iFResult;
    FIL file; // Objeto de archivo local
    UINT ui32BytesRead;
    uint32_t ui32Index;

    // 1. Validar parámetros de entrada
    if (pcFilePath == NULL || psBitmapHandler == NULL) {
        return -1;
    }

	char tempFilePath[20];	

	snprintf(tempFilePath, sizeof(tempFilePath), "%s/%s", drive, pcFilePath);

    TIVA_LOGI(TASK_NAME, "File to load: %s", tempFilePath);

    // 2. Abrir el archivo directamente con la ruta completa
    iFResult = f_open(&file, tempFilePath, FA_READ);
    if (iFResult != FR_OK) {
        TIVA_LOGE(TASK_NAME, "Failed to open file. Error: %d", iFResult);
        return ((int)iFResult);
    }

    // 3. Analizar la cabecera del Bitmap
    bitmap_Parser(&file, psBitmapHandler);
    // printBitmapHeader(&psBitmapHandler->sHeader);

    uint32_t ui32BytesPerRow = psBitmapHandler->sHeader.bitmap_width * 2;
    uint32_t ui32FileStride = ui32BytesPerRow + psBitmapHandler->sHeader.padding_bytes;
    uint32_t ui32PackedSize = ui32BytesPerRow * psBitmapHandler->sHeader.bitmap_height;

    // Validación de seguridad de tamaño
    if (ui32BuffSize > 0 && ui32PackedSize > ui32BuffSize) {
        TIVA_LOGE(TASK_NAME, "Error: Bitmap size exceeds allowed buffer size");
        f_close(&file);
        return -1;
    }

    // 4. Asignar memoria para los píxeles
    psBitmapHandler->ui8Pixels = (uint8_t *)malloc(ui32PackedSize);
    if (psBitmapHandler->ui8Pixels == NULL) {
        TIVA_LOGE(TASK_NAME, "Malloc Failed: Not enough SDRAM");
        f_close(&file);
        return FR_NOT_ENOUGH_CORE;
    }
    memset(psBitmapHandler->ui8Pixels, 0x00, ui32PackedSize);

    // 5. Mover el puntero al inicio de los datos de imagen
    f_lseek(&file, psBitmapHandler->sHeader.data_offset);

    uint32_t ui32RowByteCounter = 0;
    uint32_t ui32CurrentSourceRow = 0;
    uint32_t ui32Height = psBitmapHandler->sHeader.bitmap_height;
    uint32_t ui32TotalBytesProcessed = 0;

    // Buffer local seguro para hacer lecturas en ráfaga (chunking)
    uint8_t localReadBuf[512]; 

    // 6. Extraer píxeles invirtiendo el orden (Bottom-Up a Top-Down) y quitando el padding
    do {
        iFResult = f_read(&file, localReadBuf, sizeof(localReadBuf), &ui32BytesRead);
        if (iFResult != FR_OK) break;

        for (ui32Index = 0; ui32Index < ui32BytesRead; ui32Index++) {

            if (ui32CurrentSourceRow >= ui32Height) break; // Terminado

            // A. ¿Estamos en la porción de 'Datos' de la fila? (Omitir padding)
            if (ui32RowByteCounter < ui32BytesPerRow) {

                // B. Calcular Fila de Destino Invertida
                uint32_t ui32DestRow = (ui32Height - 1) - ui32CurrentSourceRow;

                // C. Calcular posición de la columna dentro de la fila
                uint32_t ui32DestIndex = (ui32DestRow * ui32BytesPerRow) + ui32RowByteCounter;

                if (ui32DestIndex < ui32PackedSize) {
                    psBitmapHandler->ui8Pixels[ui32DestIndex] = localReadBuf[ui32Index];
                    ui32TotalBytesProcessed++;
                }
            }

            ui32RowByteCounter++;

            // D. Si terminamos de leer la fila + su padding, pasamos a la siguiente
            if (ui32RowByteCounter >= ui32FileStride) {
                ui32RowByteCounter = 0; 
                ui32CurrentSourceRow++; 
            }
        }

        if (ui32CurrentSourceRow >= ui32Height) break; // Salir del bucle externo

    } while (ui32BytesRead == sizeof(localReadBuf));

    // 7. Limpieza y cierre
    f_close(&file);
    TIVA_LOGI(TASK_NAME, "Bitmap loaded to SDRAM successfully. Bytes processed: %lu", ui32TotalBytesProcessed);

    return 0;
}

bool FM_FetchBDF(const uint8_t drive, const char *pcFilePath, BDF_Font_t *psFont,  uint16_t startChar, uint16_t endChar) {
    FIL file;
    FRESULT fr;
    UINT bytesRead;

	if(pcFilePath == NULL) return false;

   const char *driveStr = FM_getDriveString(drive);
	if(driveStr == NULL) {
		TIVA_LOGE(TASK_NAME, "Drive specified was not found!");
		return false;
	}
	char tempFilePath[50];	

	snprintf(tempFilePath, sizeof(tempFilePath), "%s/%s", driveStr, pcFilePath);

    TIVA_LOGI(TASK_NAME, "BDF to fetch: %s", tempFilePath);

    fr = f_open(&file, tempFilePath, FA_READ);
    if (fr != FR_OK) return false;

    // 1. OBTENER TAMAÑO DEL ARCHIVO Y LEERLO COMPLETO DE GOLPE
    uint32_t fileSize = f_size(&file);
    
    // Asignamos un buffer temporal gigante en la SDRAM para todo el archivo
    char *fileBuffer = (char *)malloc(fileSize + 1);
    if (!fileBuffer) {
        f_close(&file);
        return false;
    }

  	uint32_t startTime;
  	startTime = 0;

  	startTime = GetExecTimeMs();
    // Leemos todo el archivo en una sola y masiva transacción SPI (Súper rápido)
    fr = f_read(&file, fileBuffer, fileSize, &bytesRead);
    f_close(&file); // Ya no necesitamos la SD, podemos cerrarla aquí
    TIVA_LOGI(TASK_NAME, "Tiempo de carga: %u", GetExecTimeMs() - startTime);

    if (fr != FR_OK || bytesRead != fileSize) {
        free(fileBuffer);
        return false;
    }
    fileBuffer[fileSize] = '\0'; // Aseguramos que termine en null

    // 2. INICIALIZAR ESTRUCTURAS DE LA FUENTE
    psFont->firstChar = startChar;
    psFont->lastChar = endChar;
    psFont->poolSize = 0;
    
    uint32_t numChars = endChar - startChar + 1;
    psFont->glyphs = (BDF_Glyph_t *)calloc(numChars, sizeof(BDF_Glyph_t)); // calloc limpia con ceros automáticamente
    psFont->pixelPool = (uint8_t *)malloc(65536); 

    if (!psFont->glyphs || !psFont->pixelPool) {
        free(fileBuffer);
        if(psFont->glyphs) free(psFont->glyphs);
        if(psFont->pixelPool) free(psFont->pixelPool);
        return false;
    }

    // 3. PARSEO EN RAM (A la velocidad nativa del CPU)
    int32_t currentChar = -1;
    bool inBitmap = false;
    BDF_Glyph_t tempGlyph = {0};

    char *line = fileBuffer;
    char *nextLine;

    // Recorremos el buffer en RAM línea por línea
    while (line < fileBuffer + fileSize) {
        // Encontrar el final de la línea actual y cambiarlo por \0 para aislarla
        nextLine = strchr(line, '\n');
        if (nextLine) {
            *nextLine = '\0'; 
            nextLine++; // Apuntar al inicio de la siguiente línea
            
            // Limpiar posible retorno de carro (Windows \r)
            if (nextLine > fileBuffer + 1 && *(nextLine - 2) == '\r') {
                *(nextLine - 2) = '\0';
            }
        } else {
            nextLine = fileBuffer + fileSize; // Última línea
        }

        // ==========================================
        // TU LÓGICA DE PARSEO INTACTA (Pero ahora corre en RAM)
        // ==========================================
        
        // Optimización rápida: checar la primera letra antes de hacer strncmp
        char firstChar = line[0];

        if (inBitmap) {
            if (firstChar == 'E' && strncmp(line, "ENDCHAR", 7) == 0) {
                inBitmap = false;
                currentChar = -1;
            } 
            else if (currentChar >= startChar && currentChar <= endChar) {
                int bytesPerRow = (tempGlyph.width + 7) / 8;
				int i = 0;
                for (; i < bytesPerRow; i++) {
                    psFont->pixelPool[psFont->poolSize++] = BDF_HexToByte(&line[i * 2]);
                }
            }
        } 
        else if (firstChar == 'B' && strncmp(line, "BITMAP", 6) == 0) {
            inBitmap = true;
            if (currentChar >= startChar && currentChar <= endChar) {
                uint32_t index = currentChar - startChar;
                psFont->glyphs[index] = tempGlyph;
                psFont->glyphs[index].bitmapOffset = psFont->poolSize;
                psFont->glyphs[index].yOffset = -(tempGlyph.height + tempGlyph.yOffset);
            }
        }
        else if (firstChar == 'B' && strncmp(line, "BBX", 3) == 0) {
            char *p = &line[4];
            tempGlyph.width = strtol(p, &p, 10);
            tempGlyph.height = strtol(p, &p, 10);
            tempGlyph.xOffset = strtol(p, &p, 10);
            tempGlyph.yOffset = strtol(p, NULL, 10);
        }
        else if (firstChar == 'E' && strncmp(line, "ENCODING", 8) == 0) {
            currentChar = atoi(&line[9]);
        }
        else if (firstChar == 'D' && strncmp(line, "DWIDTH", 6) == 0) {
            tempGlyph.advanceX = atoi(&line[7]);
        }
        else if (firstChar == 'F' && strncmp(line, "FONTBOUNDINGBOX", 15) == 0) {
            char *p = &line[16];
            strtol(p, &p, 10); 
            psFont->yAdvance = strtol(p, &p, 10); 
            strtol(p, &p, 10); 
            psFont->globalYOffset = strtol(p, NULL, 10); 
        }

        // Avanzar a la siguiente línea
        line = nextLine;
    }

    // 4. LIBERAR EL BUFFER GIGANTE
    free(fileBuffer);
    return true;
}

#if 0
bool FM_LoadEVEImageLegacy(const uint8_t drive, const char *pcFilePath, gfx_Image *img, uint32_t targetRamGAddr, uint32_t *imgSize) {

    FIL file;
    UINT bytesRead;

	if(pcFilePath == NULL) return false;

	char tempFilePath[50];	

   const char *driveStr = FM_getDriveString(drive);
	if(driveStr == NULL) {
		TIVA_LOGE(TASK_NAME, "Drive specified was not found!");
		return false;
	}

	snprintf(tempFilePath, sizeof(tempFilePath), "%s/%s", driveStr, pcFilePath);

    TIVA_LOGI(TASK_NAME, "Image to load: %s", tempFilePath);

    // 1. Abrir archivo y obtener tamaño exacto
    if (f_open(&file, tempFilePath, FA_READ) != FR_OK) {
        TIVA_LOGE("SDSPI", "No se encontro la imagen: %s", pcFilePath);
        return false;
    }

    uint32_t fileSize = f_size(&file);
	*imgSize = fileSize;
    
    // 2. Asignar buffer en SDRAM exactamente del tamaño del PNG comprimido
    uint8_t *sdramBuffer = (uint8_t *)malloc(fileSize);
    if (!sdramBuffer) {
        f_close(&file);
        TIVA_LOGE("SDSPI", "Error de malloc en SDRAM para PNG");
        return false;
    }

    // 3. Lectura de ráfaga (Burst read) desde la SD a la SDRAM
    f_read(&file, sdramBuffer, fileSize, &bytesRead);
    f_close(&file);

    if (bytesRead != fileSize) {
        free(sdramBuffer);
        return false;
    }

    // 4. Transferir de la SDRAM al FT81x y decodificar en RAM_G
    bool success = gfx_ImageLoadPNG(img, sdramBuffer, fileSize, targetRamGAddr);

    // 5. ¡LIBERAR LA SDRAM! Ya no la necesitamos, EVE ya decodificó los píxeles.
    free(sdramBuffer);

    return success;
}

#endif

bool FM_LoadEVEImage(const uint8_t drive, const char *pcFilePath,
                     gfx_Image *img, uint32_t targetRamGAddr,
                     uint32_t *ramgSizeBytes) {
    FIL file;
    UINT bytesRead = 0;
    const FM_EVEImageAsset_t *cachedAsset;
    gfx_Image loadedImage = {0};
    gfx_Image *loadTarget = img != NULL ? img : &loadedImage;
    FRESULT result;
    uint32_t fileSize;
    uint8_t *sdramBuffer;
    bool success;

	if(pcFilePath == NULL) return false;

    cachedAsset = FM_EVEImageFind(drive, pcFilePath);
    if (cachedAsset != NULL) {
        FM_ApplyEVEImageAssetToWidget(cachedAsset, img);
        if (ramgSizeBytes != NULL) {
            *ramgSizeBytes = cachedAsset->ramgSizeBytes;
        }
        TIVA_LOGI(TASK_NAME, "Reusing EVE image: %s @ RAM_G 0x%08lx",
                  pcFilePath, cachedAsset->ramgAddress);
        return true;
    }

	char tempFilePath[50];	

   const char *driveStr = FM_getDriveString(drive);
	if(driveStr == NULL) {
		TIVA_LOGE(TASK_NAME, "Drive specified was not found!");
		return false;
	}

	snprintf(tempFilePath, sizeof(tempFilePath), "%s/%s", driveStr, pcFilePath);

    TIVA_LOGI(TASK_NAME, "Image to load: %s", tempFilePath);

    result = f_open(&file, tempFilePath, FA_READ);
    if (result != FR_OK) {
        TIVA_LOGE(TASK_NAME, "Image not found: %s (%s)", pcFilePath,
                  FM_StringFromFResult(result));
        return false;
    }

    fileSize = f_size(&file);
    if (ramgSizeBytes != NULL) {
        *ramgSizeBytes = 0;
    }
    
    sdramBuffer = (uint8_t *)malloc(fileSize);
    if (!sdramBuffer) {
        f_close(&file);
        TIVA_LOGE(TASK_NAME, "Failed to allocate SDRAM buffer for PNG");
        return false;
    }

    result = f_read(&file, sdramBuffer, fileSize, &bytesRead);
    f_close(&file);

    if (result != FR_OK || bytesRead != fileSize) {
        free(sdramBuffer);
        TIVA_LOGE(TASK_NAME, "Image read failed: %s", pcFilePath);
        return false;
    }

    if (targetRamGAddr == 0U) {
        targetRamGAddr = g_ui32NextEVEImageRAMG;
    }

    success = gfx_ImageLoadPNG(loadTarget, sdramBuffer, fileSize,
                               targetRamGAddr);

    free(sdramBuffer);

    if (!success) {
        return false;
    }

    if (!FM_RegisterEVEImageAsset(drive, pcFilePath, loadTarget)) {
        return false;
    }

    if (ramgSizeBytes != NULL) {
        *ramgSizeBytes = loadTarget->ramgSizeBytes;
    }

    TIVA_LOGI(TASK_NAME,
              "Loaded EVE image: %s @ RAM_G 0x%08lx, %ux%u, %lu bytes",
              pcFilePath, loadTarget->ramgAddress, loadTarget->size.width,
              loadTarget->size.height, loadTarget->ramgSizeBytes);

    return true;
}

// =====================================================================
// FUNCIONES DE ESCRITURA (¡NUEVAS! Para tu Data Logging)
// =====================================================================

bool FM_WriteFile(const uint8_t drive, const char *pcFilePath, const uint8_t *pData, uint32_t size) {
    FIL file;
    UINT bytesWritten = 0;
    
    // FIX 1: Usar operador lógico OR (||)
    if(pcFilePath == NULL || pData == NULL) return false;

    // FIX 2: Buffer lo suficientemente grande (al menos 64 o 100 bytes)
    char tempFilePath[64];  
	const char *driveStr = FM_getDriveString(drive);

	if(driveStr == NULL) {
		TIVA_LOGE(TASK_NAME, "Drive specified was not found!");
		return false;
	}
	
    snprintf(tempFilePath, sizeof(tempFilePath), "%s/%s", driveStr, pcFilePath);

    TIVA_LOGI(TASK_NAME, "File to write: %s", tempFilePath);

    FRESULT res = f_open(&file, tempFilePath, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        TIVA_LOGE(TASK_NAME, "Failed to open/create file. Error: %s", FM_StringFromFResult(res));
        return false;
    } 

    res = f_write(&file, pData, size, &bytesWritten);
    f_close(&file);

    if (res != FR_OK || bytesWritten != size) {
        TIVA_LOGE(TASK_NAME, "Write failed or incomplete. Error: %s", FM_StringFromFResult(res));
        return false;
    }

    TIVA_LOGI(TASK_NAME, "File closed. Bytes written: %u", bytesWritten);
    return true;
}

bool FM_AppendLog(const uint8_t drive, const char *pcFilePath, const char *logText) {
    FIL file;
    UINT bytesWritten;

    if(pcFilePath == NULL || logText == NULL) {
        TIVA_LOGE(TASK_NAME, "Error: Append log path or text was NULL!");
        return false;
    }   

    // Increased buffer size to prevent memory corruption
    char tempFilePath[64];  
    const char *driveStr = FM_getDriveString(drive);

    if(driveStr == NULL) {
        TIVA_LOGE(TASK_NAME, "Drive specified was not found!");
        return false;
    }

    snprintf(tempFilePath, sizeof(tempFilePath), "%s/%s", driveStr, pcFilePath);

    // TIVA_LOGI(TASK_NAME, "File to append into: %s", tempFilePath);
    
    // 1. Open the file. If it doesn't exist, create it. If it does, just open it.
    FRESULT res = f_open(&file, tempFilePath, FA_WRITE | FA_OPEN_ALWAYS);
    
    if (res != FR_OK) {
        TIVA_LOGE(TASK_NAME, "Error abriendo LOG: %s", FM_StringFromFResult(res));
        return false;
    }

    // 2. Move the file read/write pointer to the end of the file (Append mode)
    res = f_lseek(&file, f_size(&file));
    if (res != FR_OK) {
        TIVA_LOGE(TASK_NAME, "Error seeking to end of LOG: %s", FM_StringFromFResult(res));
        f_close(&file);
        return false;
    }

    // 3. Write the new data at the end of the file
    res = f_write(&file, logText, strlen(logText), &bytesWritten);
    
    // 4. Force physical save to the USB/SD (Sync) and close
    f_sync(&file); 
    f_close(&file);

    return (res == FR_OK && bytesWritten == strlen(logText));
}

FRESULT FM_ProbeRoot(uint8_t drive) {
    DIR dir;
    FILINFO fileInfo;
    const char *driveString = FM_getDriveString(drive);
    char rootPath[4];
    FRESULT result;

    if (driveString == NULL ||
        snprintf(rootPath, sizeof(rootPath), "%s/", driveString) >=
            (int)sizeof(rootPath)) {
        return FR_INVALID_DRIVE;
    }

    memset(&dir, 0, sizeof(dir));
    memset(&fileInfo, 0, sizeof(fileInfo));
    result = f_opendir(&dir, rootPath);
    if (result != FR_OK) {
        return result;
    }

    /* FatFs R0.09b has no f_closedir(). Reading forces real media access. */
    return f_readdir(&dir, &fileInfo);
}

// =====================================================================
// UTILIDADES (Directorios)
// =====================================================================
int FM_ListDirectory(const char *drive, const char *pcDirPath) { 
    uint32_t ui32TotalSize = 0;
    uint32_t ui32FileCount = 0;
    uint32_t ui32DirCount = 0;
    FRESULT iFResult;
    FATFS *psFatFs;
    char *pcFileName;
    
    // 1. Variables locales en lugar de globales
    DIR sDirObject;
    FILINFO sFileInfo;

#if _USE_LFN
    char pucLfn[_MAX_LFN + 1];
    sFileInfo.lfname = pucLfn;
    sFileInfo.lfsize = sizeof(pucLfn);
#endif

	if(drive == NULL | pcDirPath == NULL) return -1;

	char tempFilePath[20];	

	snprintf(tempFilePath, sizeof(tempFilePath), "%s/%s", drive, pcDirPath);

    TIVA_LOGI(TASK_NAME, "Directory to list: %s", tempFilePath);

    // 2. Abrir el directorio usando la ruta proporcionada (Ej: "0:/" o "1:/logs")
    iFResult = f_opendir(&sDirObject, tempFilePath);
    
    if (iFResult != FR_OK) {
        TIVA_LOGE(TASK_NAME, "Error opening dir %s: %s", 
                  tempFilePath, FM_StringFromFResult(iFResult));
        return ((int)iFResult);
    }

    TIVA_LOGI(TASK_NAME, "Contents of directory: %s", tempFilePath);

    for (;;) {
        // Leer una entrada del directorio
        iFResult = f_readdir(&sDirObject, &sFileInfo);
        
        // Salir si hay error o si llegamos al final (fname vacío)
        if (iFResult != FR_OK || !sFileInfo.fname[0]) {
            break;
        }

        // Si es directorio, incrementar cuenta. Si es archivo, sumar tamaño.
        if (sFileInfo.fattrib & AM_DIR) {
            ui32DirCount++;
        } else {
            ui32FileCount++;
            ui32TotalSize += sFileInfo.fsize;
        }

#if _USE_LFN
        pcFileName = ((*sFileInfo.lfname) ? sFileInfo.lfname : sFileInfo.fname);
#else
        pcFileName = sFileInfo.fname;
#endif

        // Imprimir información de la entrada
        TIVA_LOGI(TASK_NAME, "%c%c%c%c%c %u/%02u/%02u %02u:%02u %9u  %s",
                  (sFileInfo.fattrib & AM_DIR) ? 'D' : '-',
                  (sFileInfo.fattrib & AM_RDO) ? 'R' : '-',
                  (sFileInfo.fattrib & AM_HID) ? 'H' : '-',
                  (sFileInfo.fattrib & AM_SYS) ? 'S' : '-',
                  (sFileInfo.fattrib & AM_ARC) ? 'A' : '-',
                  (sFileInfo.fdate >> 9) + 1980, (sFileInfo.fdate >> 5) & 15,
                  sFileInfo.fdate & 31, (sFileInfo.ftime >> 11),
                  (sFileInfo.ftime >> 5) & 63, sFileInfo.fsize, pcFileName);
    }

    if (iFResult != FR_OK) {
        return ((int)iFResult);
    }

    // Resumen de archivos
    TIVA_LOGI(TASK_NAME, "%4u File(s), %10u bytes total \t\t%4u Dir(s)",
              ui32FileCount, ui32TotalSize, ui32DirCount);

    // 3. Obtener espacio libre DE LA UNIDAD ESPECÍFICA (pcDirPath)
    DWORD freeClusters;
    iFResult = f_getfree(pcDirPath, &freeClusters, &psFatFs);
    
    if (iFResult == FR_OK) {
        // Cálculo correcto: (Clústeres Libres * Sectores por Clúster) / 2 = KiloBytes
        uint32_t freeSpaceKB = (freeClusters * psFatFs->csize) / 2;
        TIVA_LOGI(TASK_NAME, ", %10uK bytes free on drive", freeSpaceKB);
    } else {
        TIVA_LOGE(TASK_NAME, "Failed to get free space: %s", FM_StringFromFResult(iFResult));
    }

    return 0;
}


