
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fatfs/src/ff.h>

#include "can_id_map.h"
#include "hal_inst_can.h"
#include "event_engine.h"

#include "experiments_cfg.h"


static ul_fault_current_test_s g_sCurrentFaultTest = {0};
static ul_crush_test_s g_sCurrentCrashTest = {0};
static ul_sequence_profile_t g_sCurrentSequenceTest = {0};

// Calbacks
static void onFaultCurrentSubmitted(EventParam_t arg) {
	// TODO Check limits
	g_sCurrentFaultTest.f32TargetCurrent = arg.f32;
	// Update all listeners about the change
	Event_Post(EVT_SYS_FAULT_CFG_CURRENT, (EventParam_t){.f32 = g_sCurrentFaultTest.f32TargetCurrent});
}

static void onFaultDurationSubmitted(EventParam_t arg) {
	// TODO Check limits
	g_sCurrentFaultTest.ui16Duration = arg.f32;
	// Update all listeners about the change
	Event_Post(EVT_SYS_FAULT_CFG_DURATION, (EventParam_t){.f32 = g_sCurrentFaultTest.ui16Duration});
}

static void onFaultCaliberSubmitted(EventParam_t arg) {
	// TODO Check limits
	snprintf(g_sCurrentFaultTest.pcCaliber, sizeof(g_sCurrentFaultTest.pcCaliber), "%s", arg.str);
	// Update all listeners about the change
	Event_Post(EVT_SYS_FAULT_CFG_CALIBER, (EventParam_t){.str = g_sCurrentFaultTest.pcCaliber});
}

static void onFaultResistanceSubmitted(EventParam_t arg) {
	// TODO Check limits
	g_sCurrentFaultTest.bIsHighResistence = arg.bool_;
	// Update all listeners about the change
	//Event_Post(EVT_SYS_FAULT_CFG_IS_HIGH_RESISTENCE, (EventParam_t){.bool_ = g_sCurrentFaultTest.bIsHighResistence});
}

static void onCrushTemperatureSubmitted(EventParam_t arg) {
	// TODO Check limits
	g_sCurrentCrashTest.f32TargetTemp = arg.f32;
	// Update all listeners about the change
	Event_Post(EVT_SYS_CRUSH_CFG_TEMP, (EventParam_t){.f32 = g_sCurrentCrashTest.f32TargetTemp});
}

static void onCrushDurationSubmitted(EventParam_t arg) {
	// TODO Check limits
	g_sCurrentCrashTest.ui16Duration = arg.f32;
	// Update all listeners about the change
	Event_Post(EVT_SYS_CRUSH_CFG_DURATION, (EventParam_t){.f32 = g_sCurrentCrashTest.ui16Duration});
}

static void onCrushCaliberSubmitted(EventParam_t arg) {
	// TODO Check limits
	snprintf(g_sCurrentCrashTest.pcCaliber, sizeof(g_sCurrentCrashTest.pcCaliber), "%s", arg.str);
	// Update all listeners about the change
	Event_Post(EVT_SYS_CRUSH_CFG_CALIBER, (EventParam_t){.str = g_sCurrentCrashTest.pcCaliber});
}

static void onCrushResistanceSubmitted(EventParam_t arg) {
	// TODO Check limits
	g_sCurrentCrashTest.bIsHighResistence = arg.bool_;
	// Update all listeners about the change
	Event_Post(EVT_SYS_CRUSH_CFG_IS_HIGH_RESISTENCE, (EventParam_t){.bool_ = g_sCurrentCrashTest.bIsHighResistence});
}

static void onProfileCaliberSubmitted(EventParam_t arg) {
	snprintf(g_sCurrentSequenceTest.pcCaliber, sizeof(g_sCurrentSequenceTest.pcCaliber), "%s", arg.str);
	Event_Post(EVT_SYS_PROFILE_CFG_CALIBER, ( EventParam_t ){.str = g_sCurrentSequenceTest.pcCaliber});
}

bool ExperimentCfg_newFaultTest(uint16_t targetCurrent, uint16_t duration) {
	// g_sCurrentFaultTest.isRunning = true;
	// Its probably good idea to check if the test is already running
	g_sCurrentFaultTest.f32TargetCurrent = targetCurrent;
	g_sCurrentFaultTest.ui16Duration = duration;
	g_sCurrentFaultTest.isRunning = false;
	g_sCurrentFaultTest.bIsHighResistence = false;
	memset(g_sCurrentFaultTest.pcCaliber, 0, sizeof(g_sCurrentFaultTest.pcCaliber));

	return true;
}

bool ExperimentCfg_endFaultTest(char *endTime, char *endDate) {
	g_sCurrentFaultTest.isRunning = false;

	snprintf(g_sCurrentFaultTest.pcEndDate, 12, "%s", endDate);	
	snprintf(g_sCurrentFaultTest.pcEndTime, 12, "%s", endTime);	

	return true;
}

bool ExperimentCfg_newCrushTest(uint16_t temp, uint16_t duration) {
	g_sCurrentCrashTest.ui16Duration = duration;
	g_sCurrentCrashTest.f32TargetTemp = temp;
	g_sCurrentCrashTest.isRunning = false;
	g_sCurrentCrashTest.bIsHighResistence = false;
	memset(g_sCurrentCrashTest.pcCaliber, 0, sizeof(g_sCurrentCrashTest.pcCaliber));

	return true;
}

bool ExperimentCfg_startCrushTest(char *startTime, char *pcStartDate) {
	if(g_sCurrentCrashTest.isRunning) return false;

	g_sCurrentCrashTest.isRunning = true;

	snprintf(g_sCurrentCrashTest.pcEndDate, 12, "%s", pcStartDate);	
	snprintf(g_sCurrentCrashTest.pcEndTime, 12, "%s", startTime);	

	return true;
}

bool ExperimentCfg_endCrushTest(char *endTime, char *endDate) {
	g_sCurrentCrashTest.isRunning = false;

	snprintf(g_sCurrentCrashTest.pcEndDate, 12, "%s", endDate);	
	snprintf(g_sCurrentCrashTest.pcEndTime, 12, "%s", endTime);	

	return true;
}

ul_crush_test_s	ExperimentCfg_getCurrCrushCfg(void) {
	return g_sCurrentCrashTest;
}

ul_fault_current_test_s ExperimentCfg_getCurrFaultCfg(void) {
	return g_sCurrentFaultTest;
}

void ExperimentCfg_init(void) {
	Event_Subscribe(EVT_SYS_FAULT_CFG_SUBMIT_CURRENT, (EventHandler_fn)onFaultCurrentSubmitted);
	Event_Subscribe(EVT_SYS_FAULT_CFG_SUBMIT_DURATION, (EventHandler_fn)onFaultDurationSubmitted);
	Event_Subscribe(EVT_SYS_FAULT_CFG_SUBMIT_CALIBER, (EventHandler_fn)onFaultCaliberSubmitted);
	Event_Subscribe(EVT_SYS_FAULT_CFG_SUBMIT_IS_HIGH_RESISTENCE, (EventHandler_fn)onFaultResistanceSubmitted);

	Event_Subscribe(EVT_SYS_CRUSH_CFG_SUBMIT_TEMP, (EventHandler_fn)onCrushTemperatureSubmitted);
	Event_Subscribe(EVT_SYS_CRUSH_CFG_SUBMIT_DURATION, (EventHandler_fn)onCrushDurationSubmitted);
	Event_Subscribe(EVT_SYS_CRUSH_CFG_SUBMIT_CALIBER, (EventHandler_fn)onCrushCaliberSubmitted);
	Event_Subscribe(EVT_SYS_CRUSH_CFG_SUBMIT_IS_HIGH_RESISTENCE, (EventHandler_fn)onCrushResistanceSubmitted);

	Event_Subscribe(EVT_SYS_PROFILE_CFG_SUBMIT_CALIBER, (EventHandler_fn)onProfileCaliberSubmitted);
}

// Delimitadores permitidos en el CSV (coma, espacio, tabulador, retorno de carro, salto de línea)
#define CSV_DELIMITERS " ,\t\r\n"

#define UNINITIALIZED_MIN_CURRENT 9999.0f

bool ExperimentCfg_parseSequence(const char *filePath, ul_sequence_profile_t *profile) {
	FIL fp;
    FRESULT res;
    char lineBuffer[80]; // Buffer temporal para leer una línea a la vez

    // 1. Inicializar y limpiar el perfil
    // memset(profile, 0, sizeof(ModeCProfile_t));
    strncpy(profile->absoluteFilePath, filePath, MAX_PROFILE_NAME_LEN - 1);
    
    // Inicialización de metadatos
    profile->maxCurrentRequested = 0.0f;
    profile->minCurrentRequested = UNINITIALIZED_MIN_CURRENT;
    profile->totalDurationSec = 0;
    profile->numSegments = 0;
    profile->bIsValid = false;

    // 2. Abrir el archivo CSV en modo lectura
    res = f_open(&fp, filePath, FA_READ);
    if (res != FR_OK) {
        return false;
    }

    // 3. Leer y parsear línea por línea
    while (f_gets(lineBuffer, sizeof(lineBuffer), &fp) != NULL) {
        
        // Ignorar comentarios (líneas que empiecen con '#' o '/') y líneas vacías
        if (lineBuffer[0] == '#' || lineBuffer[0] == '/' || lineBuffer[0] == '\n' || lineBuffer[0] == '\r') {
            continue;
        }

        // Proteger contra desbordamiento de RAM
        if (profile->numSegments >= MAX_TEST_SEGMENTS) {
            break; 
        }

        char *endptr; // Puntero para la validación estricta de strtof/strtoul

        // ==========================================
        // TOKEN 1: COMANDO (RAMP, STEP, HOLD)
        // ==========================================
        char *token = strtok(lineBuffer, CSV_DELIMITERS);
        if (token == NULL) continue; 
        
        // Ignorar encabezados
        if (strcmp(token, "TIPO") == 0 || strcmp(token, "TYPE") == 0) continue;

        ul_sequence_segment_t *seg = &profile->segments[profile->numSegments];

        if (strcmp(token, "RAMP") == 0) seg->transition = SEG_RAMP;
        else if (strcmp(token, "STEP") == 0) seg->transition = SEG_STEP;
        else if (strcmp(token, "HOLD") == 0) seg->transition = SEG_HOLD;
        else {
            // ERROR: Comando desconocido ("PASO", "RMP", etc.)
            f_close(&fp);
            return false; 
        }

        // ==========================================
        // TOKEN 2: CORRIENTE OBJETIVO (float)
        // ==========================================
        token = strtok(NULL, CSV_DELIMITERS);
        if (token == NULL) {
            f_close(&fp); return false; // ERROR: Faltan columnas
        }
        
        // strtof con validación de tipo
        float tempCurrent = strtof(token, &endptr);
        
        // Si endptr no apunta al terminador, había caracteres inválidos (ej. "120A")
        if (*endptr != '\0' && *endptr != '\r' && *endptr != '\n') {
            f_close(&fp); return false; 
        }
        
        // Lógica de Negocio: Validar límites físicos del hardware (0 a 500A)
        if (tempCurrent < 0.0f || tempCurrent > MAX_ALLOWED_CURRENT) {
            f_close(&fp); return false; 
        }
        
        seg->targetCurrent = tempCurrent;

        // ==========================================
        // TOKEN 3: DURACIÓN EN SEGUNDOS (uint32)
        // ==========================================
        token = strtok(NULL, CSV_DELIMITERS);
        if (token == NULL) {
            f_close(&fp); return false; // ERROR: Falta la columna de tiempo
        }

        uint32_t tempDuration = (uint32_t)strtoul(token, &endptr, 10);
        
        if (*endptr != '\0' && *endptr != '\r' && *endptr != '\n') {
            f_close(&fp); return false; // ERROR: Tiempo contiene basura alfanumérica
        }

        // Un paso no puede durar 0 segundos (colapsaría el controlador de control)
        if (tempDuration < 1) {
            f_close(&fp); return false; 
        }

        seg->durationSec = tempDuration;

        // ==========================================
        // VALIDACIÓN: COLUMNAS SOBRANTES
        // ==========================================
        token = strtok(NULL, CSV_DELIMITERS);
        if (token != NULL && token[0] != '\r' && token[0] != '\n' && token[0] != '#') {
             f_close(&fp); return false; // ERROR: Hay 4 o más columnas en esta línea
        }

        // ==========================================
        // ACTUALIZACIÓN DE METADATOS Y LÍMITES
        // ==========================================
        profile->totalDurationSec += seg->durationSec;
        
        // En los comandos HOLD ignoramos targetCurrent ya que la máquina debe mantener la corriente del paso anterior
        if (seg->transition != SEG_HOLD) {
            
            if (seg->targetCurrent > profile->maxCurrentRequested) {
                profile->maxCurrentRequested = seg->targetCurrent;
            }
            
            if (seg->targetCurrent < profile->minCurrentRequested) {
                profile->minCurrentRequested = seg->targetCurrent;
            }
        }

        profile->numSegments++;
    }

    // 4. Cerrar el archivo correctamente
    f_close(&fp);

    // 5. VALIDACIÓN FINAL Y LIMPIEZA
    if (profile->numSegments > 0) {
        
        // Si el archivo CSV solo contenía comandos HOLD, minCurrent no se habrá actualizado.
        // Se asegura un valor neutro para no exportar UNINITIALIZED_MIN_CURRENT.
        if (profile->minCurrentRequested == UNINITIALIZED_MIN_CURRENT) {
            profile->minCurrentRequested = 0.0f;
        }
        
        profile->bIsValid = true;
    }

    return profile->bIsValid;
}

bool ExperimentCfg_newSequenceTest(const char *filePath) {
	if(filePath == NULL) return false;
	
	bool ret = ExperimentCfg_parseSequence(filePath, &g_sCurrentSequenceTest);
	if(!ret) {
		return false;
	}

	snprintf(g_sCurrentSequenceTest.pcCaliber, sizeof(g_sCurrentSequenceTest.pcCaliber), "%s", "00");
	return true;
}

bool ExperimentCfg_startSequenceTest(char *startTime, char *pcStartDate) {
    // 1. Validar que realmente hay un perfil válido cargado en memoria
    if (!g_sCurrentSequenceTest.bIsValid) {
        return false;
    }

    // 2. Registrar el Timestamp de inicio (Crucial para tu archivo de exportación FAT)
    if (pcStartDate != NULL) snprintf(g_sCurrentSequenceTest.pcStartDate, 12, "%s", pcStartDate);    
    if (startTime != NULL) snprintf(g_sCurrentSequenceTest.pcStartTime, 12, "%s", startTime);    

    // NOTA: Si en el futuro agregas un 'isRunning' a ul_sequence_profile_t, lo activarías aquí.
    return true;
}

bool ExperimentCfg_stopSequenceTest(char *endTime, char *endDate) {
    // Registrar el Timestamp de finalización
    if (endDate != NULL) snprintf(g_sCurrentSequenceTest.pcEndDate, 12, "%s", endDate);    
    if (endTime != NULL) snprintf(g_sCurrentSequenceTest.pcEndTime, 12, "%s", endTime);    

    return true;
}

bool ExperimentCfg_getSequenceSegment(uint16_t index, ul_sequence_segment_t *segment) {
    // CORRECCIÓN 1: El índice es base 0, por lo que debe ser '>=' (mayor o igual) a numSegments.
    // Además, validamos que el puntero 'segment' no sea nulo.
    if (!g_sCurrentSequenceTest.bIsValid || index >= g_sCurrentSequenceTest.numSegments || segment == NULL) {
        return false;
    }
    
    // CORRECCIÓN 2: Asignar el índice específico del arreglo, NO la estructura de perfil completa.
    // Antes tenías: *segment = g_sCurrentSequenceTest; (Error de tipos)
    *segment = g_sCurrentSequenceTest.segments[index];
    
    return true;
}

// NUEVO: Getter para obtener todo el perfil (Lo necesitarás para la gráfica de Vista Previa)
const ul_sequence_profile_t* ExperimentCfg_getCurrSequenceCfg(void) {
	if(g_sCurrentSequenceTest.bIsValid)
    	return (const ul_sequence_profile_t *)&g_sCurrentSequenceTest;
	
	return NULL;
}

void ExperimentCfg_generateSequencePoints(float *data, uint16_t size) {
    const ul_sequence_profile_t *profile = (const ul_sequence_profile_t *)&g_sCurrentSequenceTest;

    // Si la duración es 0 (error de parseo), limpiar gráfica
    //if (profile->totalDurationSec == 0) {
    //    return;
    //}

    // Iterar para rellenar los 400 pixeles/puntos de la pantalla
	int i = 0;
    for (; i < size; i++) {
        
        // ¿A qué segundo del ensayo corresponde el pixel "i"?
        float t = (float)i * (float)profile->totalDurationSec / (size - 1);
        
        float val = 0.0f;
        float time_tracking = 0.0f;
        float curr_tracking = 0.0f; // La corriente al inicio del segmento evaluado
        
        // Buscar en qué segmento cae el tiempo 't'
		int s = 0;
        for (; s < profile->numSegments; s++) {
            ul_sequence_segment_t seg = profile->segments[s];
            float seg_end_t = time_tracking + seg.durationSec;
            
            float target_c = seg.targetCurrent;
            if (seg.transition == SEG_HOLD) target_c = curr_tracking;
            
            // Si el tiempo 't' está dentro de este segmento
            if (t >= time_tracking && t <= seg_end_t) {
                if (seg.transition == SEG_STEP || seg.transition == SEG_HOLD) {
                    val = target_c;
                } 
                else if (seg.transition == SEG_RAMP) {
                    // Interpolación lineal
                    float progress = (t - time_tracking) / seg.durationSec;
                    val = curr_tracking + (target_c - curr_tracking) * progress;
                }
                break;
            }
            
            // Avanzar acumuladores para evaluar el siguiente segmento
            time_tracking = seg_end_t;
            curr_tracking = target_c;
            
            // Para proteger el último punto debido a decimales
            val = curr_tracking; 
        }
        
        data[i] = val;
    }
}

// Función para enviar el comando de arranque a la instrumentación
void ExperimentCfg_SendStartCommand(uint8_t testType, uint16_t durationSec, float targetValue, bool isHighRes) {
    union { float f; uint8_t bytes[4]; } targetData;
    targetData.f = targetValue;

    HAL_CAN_Msg_t startMsg;
    startMsg.id = CAN_ID_REQ_START_TEST;
    startMsg.isExtended = false;
    startMsg.length = 8;
    
    startMsg.data[0] = testType;
    startMsg.data[1] = (durationSec >> 8) & 0xFF; // MSB
    startMsg.data[2] = durationSec & 0xFF;        // LSB
    startMsg.data[3] = targetData.bytes[0];
    startMsg.data[4] = targetData.bytes[1];
    startMsg.data[5] = targetData.bytes[2];
    startMsg.data[6] = targetData.bytes[3];
    startMsg.data[7] = isHighRes ? 1 : 0;         // Flags adicionales
    
    HAL_CAN_Transmit(&startMsg);
}

// Función para detener la instrumentación
void ExperimentCfg_SendStopCommand(void) {
    HAL_CAN_Msg_t stopMsg;
    stopMsg.id = CAN_ID_REQ_STOP_TEST;
    stopMsg.isExtended = false;
    stopMsg.length = 1;
    stopMsg.data[0] = 0xFF; // Payload genérico de paro de emergencia / fin
    
    HAL_CAN_Transmit(&stopMsg);
}

