#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

// =========================================================================
// ESTRUCTURA DE DATOS PARA EL LOG
// =========================================================================
typedef struct {
    uint32_t timeMs;          // Tiempo transcurrido de la prueba (milisegundos)
    float voltagePrimary;
    float voltageSecondary;
    float currentPrimary;
    float currentSecondary;
    float tempProbeMain;
    float tempProbeSec;
    float tempTxPrimary;
    float tempTxSecondary;
    float tempCableA;
    float tempCableB;         // Nota: lo ajusté para coincidir con tu dashboard
    float tempCJC;            
} LogDataRow_t;

typedef struct {
    char dateStart[16];
    char timeStart[16];
    char timeEnd[16];
    
    uint32_t testType;           // 0: FAULT, 1: CRUSH, 2: SEQUENCE
    uint32_t setDurationSec;
    uint32_t realDurationSec;
    bool isHighResistance;

    // Parámetros dinámicos según el tipo de prueba
    float targetCurrent;         // Para FAULT
    float presetVoltage;         // Para FAULT
    float targetTemp;            // Para CRUSH
    float maxCurrentReq;         // Para SEQUENCE
    uint16_t numSegments;        // Para SEQUENCE
    char profileFilename[64];    // Para SEQUENCE
} LogTestSummary_t;

// =========================================================================
// PROTOTIPOS PÚBLICOS
// =========================================================================

/**
 * @brief Anexa el bloque de resumen al final del archivo CSV.
 * @param summary Puntero a la estructura poblada con los datos del resumen.
 * @return true si se escribió correctamente.
 */
bool LogManager_WriteSummary(const LogTestSummary_t *summary);

/**
 * @brief Crea un nuevo archivo CSV y escribe el encabezado de las columnas.
 * @param fileName Ruta del archivo en la USB (ej. "1:/LOGS/TEST_A_001.CSV")
 * @return true si se creó exitosamente.
 */
bool LogManager_StartLog(const char *fileName);

/**
 * @brief Escribe una fila de datos en formato CSV y fuerza el guardado (sync).
 * @param data Puntero a la estructura con los datos del instante actual.
 * @return true si se escribió correctamente.
 */
bool LogManager_WriteRow(const LogDataRow_t *data);

/**
 * @brief Cierra el archivo y detiene el registro.
 */
void LogManager_StopLog(void);

/**
 * @brief Indica si el sistema está registrando datos activamente.
 */
bool LogManager_IsLogging(void);

#endif // LOG_MANAGER_H
