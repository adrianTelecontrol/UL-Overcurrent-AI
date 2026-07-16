#ifndef EXPERIMENTS_CFG_H_
#define EXPERIMENTS_CFG_H_

#include <stdint.h>
#include <stdbool.h>

#define UL_FAULT_DEFAULT_DURATION_SEC	0.0f
#define UL_FAULT_DEFAULT_CURRENT		0.0f
#define UL_CRUSH_DEFAULT_DURATION_SEC	0.0f
#define UL_CRUSH_DEFAULT_TEMP			0.0f

#define UL_TEST_FAULT		0
#define UL_TEST_CRUSH		1
#define UL_TEST_SEQUENCE	2

#define MAX_PROFILE_NAME_LEN 64
#define MAX_TEST_SEGMENTS    100  	// Capacidad máxima de pasos por ensayo
#define MAX_ALLOWED_CURRENT  500.0f // Límite físico del transformador (Spec: 500A)

typedef enum
{
    TEST_STATE_IDLE = 0,
    TEST_STATE_RUNNING,
    TEST_STATE_FINISHED,
    TEST_STATE_ABORTED
} test_manager_state_t;

typedef struct {
	float f32TargetCurrent;
	uint16_t ui16Duration;

	bool isRunning;
	bool bIsHighResistence;

	char pcCaliber[12];
	char pcStartTime[12];
	char pcStartDate[12];
	char pcEndTime[12];
	char pcEndDate[12];
} ul_fault_current_test_s;

typedef struct {
	uint16_t ui16Duration;
	float f32TargetTemp;

	bool isRunning;
	bool bIsHighResistence;

	char pcCaliber[12];
	char pcStartTime[12];
	char pcStartDate[12];
	char pcEndTime[12];
	char pcEndDate[12];
} ul_crush_test_s;

// Tipos de transición para llegar a la corriente objetivo
typedef enum {
    SEG_RAMP, // Sube o baja linealmente hasta la corriente objetivo durante la duración
    SEG_STEP, // Salta inmediatamente a la corriente objetivo y la mantiene
    SEG_HOLD  // Ignora la corriente objetivo, mantiene la corriente actual
} ul_segment_transition_e;

// Un solo bloque de instrucción
typedef struct {
    ul_segment_transition_e transition;
    float targetCurrent;     // Corriente objetivo en Amperios
    uint32_t durationSec;    // Duración en segundos
} ul_sequence_segment_t;

// El Perfil completo cargado en RAM
typedef struct {
    char absoluteFilePath[MAX_PROFILE_NAME_LEN];
    uint16_t numSegments;
    
    // Metadatos calculados durante el parseo (útiles para dibujar la vista previa)
    uint32_t totalDurationSec; 
    float maxCurrentRequested; 
	float minCurrentRequested;
    
    // Arreglo estático de segmentos
    ul_sequence_segment_t segments[MAX_TEST_SEGMENTS];
    
    bool bIsValid; // True si el archivo se leyó bien y no viola los límites físicos
	bool bIsHighResistence;

	char pcCaliber[12];
	char pcStartTime[12];
	char pcStartDate[12];
	char pcEndTime[12];
	char pcEndDate[12];
} ul_sequence_profile_t;

// Función pública
bool ExperimentCfg_parseSequence(const char *filePath, ul_sequence_profile_t *profile);
bool ExperimentCfg_newSequenceTest(const char *filePath);
bool ExperimentCfg_startSequenceTest(char *startTime, char *pcStartDate);
bool ExperimentCfg_stopSequenceTest(char *endTime, char *endDate);

bool ExperimentCfg_newFaultTest(uint16_t targetCurrent, uint16_t duration);
bool ExperimentCfg_startFaultTest(char *startTime, char *pcStartDate);
bool ExperimentCfg_endFaultTest(char *endTime, char *endDate);

bool ExperimentCfg_newCrushTest(uint16_t temp, uint16_t duration);
bool ExperimentCfg_startCrushTest(char *startTime, char *pcStartDate);
bool ExperimentCfg_endCrushTest(char *endTime, char *endDate);

ul_fault_current_test_s ExperimentCfg_getCurrFaultCfg(void);
ul_crush_test_s	ExperimentCfg_getCurrCrushCfg(void);
const ul_sequence_profile_t* ExperimentCfg_getCurrSequenceCfg(void);
bool ExperimentCfg_getSequenceSegment(uint16_t index, ul_sequence_segment_t *segment);
void ExperimentCfg_generateSequencePoints(float *data, uint16_t size);
void ExperimentCfg_SendStartCommand(uint8_t testType, uint16_t durationSec, float targetValue, bool isHighRes);
void ExperimentCfg_SendStopCommand(void);

void ExperimentCfg_init(void);

#endif // EXPERIMENTS_CFG_H_

