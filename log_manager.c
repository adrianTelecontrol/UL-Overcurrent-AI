#include "log_manager.h"
#include <stdio.h>
#include <string.h>

#include <fatfs/src/ff.h>

// =========================================================================
// VARIABLES PRIVADAS
// =========================================================================
static FIL g_logFile;
static bool g_bIsLogging = false;

// =========================================================================
// IMPLEMENTACIÓN
// =========================================================================

bool LogManager_StartLog(const char *fileName) {
    if (g_bIsLogging) {
        return false; // Ya hay un log en curso
    }

    // 1. Extraer la ruta del directorio del fileName
    // Ej: Si fileName es "1:/LOGS/FLT_...CSV", dirPath será "1:/LOGS"
    char dirPath[64];
    strncpy(dirPath, fileName, sizeof(dirPath) - 1);
    dirPath[sizeof(dirPath) - 1] = '\0'; // Asegurar terminación nula
    
    char *lastSlash = strrchr(dirPath, '/');
    if (lastSlash != NULL) {
        *lastSlash = '\0'; // Cortar el string en el último '/' para aislar la carpeta
        
        // 2. Crear el directorio
        // Si ya existe, FatFs retorna FR_EXIST, lo cual ignoramos porque es el estado deseado.
        f_mkdir(dirPath); 
    }

    // 3. Abrir / Crear el archivo
    FRESULT res = f_open(&g_logFile, fileName, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        return false;
    }

    // 4. Escribir el encabezado del CSV
    const char *header = "Time[ms],V_Pri[V],V_Sec[V],I_Pri[A],I_Sec[A],T_ProbeMain[C],T_ProbeSec[C],T_TxPri[C],T_TxSec[C],T_CableA[C],T_CableB[C],T_CJC[C]\n";
    UINT bytesWritten;
    
    res = f_write(&g_logFile, header, strlen(header), &bytesWritten);
    if (res != FR_OK || bytesWritten < strlen(header)) {
        f_close(&g_logFile);
        return false;
    }

    // 5. Asegurar que el encabezado se guarda en el disco físico inmediatamente
    f_sync(&g_logFile); 
    g_bIsLogging = true;
    
    return true;
}

bool LogManager_WriteRow(const LogDataRow_t *data) {
    if (!g_bIsLogging || data == NULL) {
        return false;
    }

    char buffer[256];
    
    // Formatear la línea CSV (usamos .2f para 2 decimales de precisión en flotantes)
    snprintf(buffer, sizeof(buffer), "%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
             data->timeMs,
             data->voltagePrimary,
             data->voltageSecondary,
             data->currentPrimary,
             data->currentSecondary,
             data->tempProbeMain,
             data->tempProbeSec,
             data->tempTxPrimary,
             data->tempTxSecondary,
             data->tempCableA,
             data->tempCableB,
             data->tempCJC);

    UINT bytesWritten;
    FRESULT res = f_write(&g_logFile, buffer, strlen(buffer), &bytesWritten);
    
    if (res != FR_OK || bytesWritten < strlen(buffer)) {
        return false; // Error de escritura (ej. USB desconectada abruptamente)
    }

    // Sincronizar con el disco tras cada línea. 
    // Crucial para pruebas de larga duración o fallas destructivas.
    f_sync(&g_logFile);
    
    return true;
}

bool LogManager_WriteSummary(const LogTestSummary_t *summary) {
    if (!g_bIsLogging || summary == NULL) {
        return false;
    }

    char buffer[512];
    int len = 0;

    // Encabezado del bloque
    len += snprintf(buffer + len, sizeof(buffer) - len, "\n--- RESUMEN DE PRUEBA ---\n");
    len += snprintf(buffer + len, sizeof(buffer) - len, "FECHA_INICIO,%s\n", summary->dateStart);
    len += snprintf(buffer + len, sizeof(buffer) - len, "HORA_INICIO,%s\n", summary->timeStart);
    len += snprintf(buffer + len, sizeof(buffer) - len, "HORA_FIN,%s\n", summary->timeEnd);

    // Mapeo del nombre de la prueba
    const char* testNames[] = {"FAULT_CURRENT", "CRUSH_TEST", "SEQUENCE_PROFILE"};
    const char* typeStr = (summary->testType <= 2) ? testNames[summary->testType] : "DESCONOCIDO";
    len += snprintf(buffer + len, sizeof(buffer) - len, "TIPO_ENSAYO,%s\n", typeStr);

    // Datos comunes
    if (summary->testType == 2) {
        len += snprintf(buffer + len, sizeof(buffer) - len, "DURACION_PROGRAMADA_S,VARIABLE\n");
    } else {
        len += snprintf(buffer + len, sizeof(buffer) - len, "DURACION_PROGRAMADA_S,%lu\n", summary->setDurationSec);
    }
    
    len += snprintf(buffer + len, sizeof(buffer) - len, "DURACION_REAL_S,%lu\n", summary->realDurationSec);
    len += snprintf(buffer + len, sizeof(buffer) - len, "ALTA_RESISTENCIA,%s\n", summary->isHighResistance ? "SI" : "NO");

    // Datos Específicos
    if (summary->testType == 0) { // FAULT
        len += snprintf(buffer + len, sizeof(buffer) - len, "CORRIENTE_OBJETIVO_A,%.2f\n", summary->targetCurrent);
        len += snprintf(buffer + len, sizeof(buffer) - len, "VOLTAJE_PRESET_V,%.2f\n", summary->presetVoltage);
    } else if (summary->testType == 1) { // CRUSH
        len += snprintf(buffer + len, sizeof(buffer) - len, "TEMPERATURA_OBJETIVO_C,%.2f\n", summary->targetTemp);
    } else if (summary->testType == 2) { // SEQUENCE
        len += snprintf(buffer + len, sizeof(buffer) - len, "CORRIENTE_MAXIMA_REQ_A,%.2f\n", summary->maxCurrentReq);
        len += snprintf(buffer + len, sizeof(buffer) - len, "NUM_SEGMENTOS,%u\n", summary->numSegments);
        len += snprintf(buffer + len, sizeof(buffer) - len, "ARCHIVO_PERFIL,%s\n", summary->profileFilename);
    }

    // Escribir en la USB y forzar el guardado
    UINT bytesWritten;
    FRESULT res = f_write(&g_logFile, buffer, strlen(buffer), &bytesWritten);
    f_sync(&g_logFile);

    return (res == FR_OK && bytesWritten == strlen(buffer));
}

void LogManager_StopLog(void) {
    if (g_bIsLogging) {
        f_close(&g_logFile);
        g_bIsLogging = false;
    }
}

bool LogManager_IsLogging(void) {
    return g_bIsLogging;
}


