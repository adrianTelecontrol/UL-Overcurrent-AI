#ifndef CFG_IMPORT_RESULT_FORM_H
#define CFG_IMPORT_RESULT_FORM_H

#include "event_engine.h"

// ========================================================
// ESTADOS DE RESULTADO DE IMPORTACIÓN
// ========================================================
typedef enum {
    CFG_RESULT_SUCCESS = 0,     // 1. Archivo enviado e Instrumentación respondió OK
    CFG_RESULT_ERR_FILE,        // 2. Archivo sin Magic Number o tamaño inválido
    CFG_RESULT_ERR_REJECTED,    // 3. Instrumentación respondió NACK (Error de Checksum o formato)
    CFG_RESULT_ERR_TIMEOUT      // 4. Se envió, pero la Instrumentación nunca respondió
} CfgImportResult_e;

extern int16_t g_i16CfgImportResultFormIndex;

// Inicializador del formulario
void initCfgImportResultForm(void);

#endif // CFG_IMPORT_RESULT_FORM_H