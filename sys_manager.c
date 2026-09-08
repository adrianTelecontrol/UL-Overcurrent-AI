
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "fatfs/src/ff.h"

#include "eeprom_map.h"
#include "hal_eeprom.h"
#include "gesture_engine.h"
#include "rtc_module.h"
#include "event_engine.h"
#include "helpers.h" // For GetExecTimeMs()
#include "can_id_map.h"
#include "hal_inst_can.h"
#include "experiments_cfg.h"
#include "log_manager.h"
#include "data_model.h"

#include "sys_manager.h"

#define CFG_EXPORT_TIMEOUT_MS	10000
#define CFG_IMPORT_TIMEOUT_MS	10000
#define PROFILE_UPDATE_PERIOD_MS 500

static SysState_e g_systemState = SYS_BOOT_INIT_START;
static LogTestSummary_t g_testSummary = {0};
static uint32_t g_stateTimer = 0;
static uint32_t g_bootStartTimer = 0;
static bool g_bWaitingInstHandshake = false;
static bool g_bInstHandshakeOk = false;
static bool g_bInstConfirmedStart = false;
static uint32_t g_ui32CurrentTestType = 0;
static uint32_t ui32TestStartTime = 0;
static uint32_t ui32LastLogTime = 0;
static uint16_t logIntervalMs = 0;
static uint32_t ui32TestDurationMs = 0;
static uint32_t ui32LastProfileUpdateMs = 0;
static uint16_t ui16CurrentSegmentIdx = 0;
static uint32_t ui32SegmentStartTimeMs = 0;
static float f32SegmentStartCurrent = 0.0f;
static float f32CurrentProfileSetpoint = 0.0f;
static uint32_t ui32LastProgressSec = 0xFFFFFFFF;
static float g_f32TargetTemp = 0.0f;
static bool g_bTargetReached = false;
static uint32_t phaseElapsedMs = 0; 

// --------- BOOT variables -------------------------
static char dateStr[11] = "27/04/2026";
static char timeStr[9] = "03:34:34";

static char globalTimeStr[9] = "03:34:34"; // Delete later

static char eepromTimestamp[8] = "[0.000]";
static char batteryTimestamp[8] = "[0.000]";
static char touchTimestamp[8] = "[0.000]";
static char instTimestamp[8] = "[0.000]";
static char dateTimestamp[8] = "[0.000]";
static char timeTimestamp[8] = "[0.000]";
static char szTimeRemaining[10] = "00:00";
static char szCurrentLogFileName[64] = {0};
static char szStabilityTime[16] = "00:00:00"; // <--- NUEVA
static uint32_t ui32StabilizationTimeMs = 0;  // <--- NUEVA
static float tempWithTimestamp[3] = {0};
static float currentWithTimestamp[3] = {0};

uint32_t ui32Countdown = 0;

bool g_bIsAutoSeqOn = true;



static void onStopAutoSeqEvent(EventParam_t arg) {
	if(g_systemState == SYS_BOOT_WAIT_5_SECONDS)
		g_bIsAutoSeqOn = false;
}

static void onContinueToDashboardEvent(EventParam_t arg) {
	if(g_systemState == SYS_BOOT_WAIT_5_SECONDS)
		g_systemState = SYS_SHOW_DASHBOARD_FORM;
}

static void onInstHandshakeOk(EventParam_t arg) {
	g_bInstHandshakeOk = true;
}

static void onImportTimeoutStart(EventParam_t arg) {
	if(g_systemState == SYS_IDLE)
		g_systemState = SYS_CFG_IMPORT_START_TIMEOUT;
}

static void onImportCfgResponded(EventParam_t arg) {
	if(g_systemState == SYS_CFG_IMPORT_TIMEOUT || g_systemState == SYS_CFG_IMPORT_START_TIMEOUT)
		g_systemState =SYS_IDLE;
}

static void onStartTestEvent(EventParam_t arg) {
	if(g_systemState == SYS_IDLE) {
		g_systemState = SYS_TEST_INIT;
		g_ui32CurrentTestType = arg.ui32;
	}
}

static void onTestStartedSuccessfully(EventParam_t arg) {
	if(g_systemState == SYS_TEST_WAIT_CONFIRM) {
		//g_systemState = SYS_TEST_RUNNING;
		g_bInstConfirmedStart = true;
	}
}

static void onTestFinished(EventParam_t arg) {
	if(g_systemState == SYS_TEST_RUNNING) {
		g_systemState = SYS_TEST_STOP;
	}
}


static void onNewCurrentValue(EventParam_t arg) {
	currentWithTimestamp[0] = arg.f32;
	currentWithTimestamp[1] = (float)phaseElapsedMs;
	currentWithTimestamp[2] = (float)ui32TestDurationMs;
	Event_Post(EVT_UI_GRAPH_CURRENT_VALUE, ( EventParam_t ){.ptr = (void *)currentWithTimestamp});
}

static void onNewTempValue(EventParam_t arg) {
	tempWithTimestamp[0] = arg.f32;
	tempWithTimestamp[1] = (float)phaseElapsedMs;
	tempWithTimestamp[2] = (float)ui32TestDurationMs;
	Event_Post(EVT_UI_GRAPH_TEMP_VALUE, ( EventParam_t ){.ptr = (void *)tempWithTimestamp});
}

static void onTestEmergencyStopEvent(EventParam_t arg) {

	if(g_systemState == SYS_TEST_RUNNING || g_systemState == SYS_TEST_WAIT_CONFIRM || g_systemState == SYS_TEST_INIT) {
		g_systemState = SYS_EMERGENCY_STOP;
	}
}

void SysManager_Init(void) {
    g_systemState = SYS_BOOT_INIT_START;
	
	Event_Subscribe(EVT_SYS_BOOT_STOP_AUTO_SEQ, (EventHandler_fn)onStopAutoSeqEvent);
	Event_Subscribe(EVT_SYS_BOOT_CONTINUE_TO_DASHBOARD, ( EventHandler_fn )onContinueToDashboardEvent);
	Event_Subscribe(EVT_SYS_BOOT_HANDSHAKE_OK, (EventHandler_fn)onInstHandshakeOk);
	Event_Subscribe(EVT_SYS_SHOW_ADJ_CFG_IMPORT_RESULT, (EventHandler_fn)onImportTimeoutStart);
	Event_Subscribe(EVT_CAN_INST_CFG_LOAD_OK, (EventHandler_fn)onImportCfgResponded);
	Event_Subscribe(EVT_CAN_INST_CFG_LOAD_ERROR, (EventHandler_fn)onImportCfgResponded);
	Event_Subscribe(EVT_SYS_SHOW_ADJ_EXPORT_CFG, (EventHandler_fn)onImportCfgResponded);
	Event_Subscribe(EVT_SYS_START_TEST, (EventHandler_fn)onStartTestEvent);
	Event_Subscribe(EVT_CAN_INST_TEST_START_OK, (EventHandler_fn)onTestStartedSuccessfully);
	Event_Subscribe(EVT_CAN_INST_TEST_FINISHED, ( EventHandler_fn )onTestFinished);
	Event_Subscribe(EVT_SYS_TEST_EMERGENCY_STOPPED, (EventHandler_fn)onTestEmergencyStopEvent);
	//Event_Subscribe(EVT_CAN_INST_TEST_START_FAIL, ( EventHandler_fn )onTestStartFail);

    Event_Subscribe(EVT_CAN_INST_CURRENT_SECUNDARY, (EventHandler_fn)onNewCurrentValue);
    Event_Subscribe(EVT_CAN_INST_TEMP_PROBE_MAIN, (EventHandler_fn)onNewTempValue);
}

void SysManager_Task(void) {
	uint32_t deltaTime = 0;
	static uint32_t ui32ProcStartTime = 0;
	static uint32_t ui32LastStateExitTime = 0;
	
    switch (g_systemState) {
        case SYS_BOOT_INIT_START:
            g_bootStartTimer = GetExecTimeMs();
			Event_Post(EVT_SYS_BOOT_CHECK_START, (EventParam_t){.ptr = NULL});
            g_systemState = SYS_BOOT_CHECK_EEPROM;
            break;

		case SYS_BOOT_CHECK_EEPROM:
			// Wait for while first
    		g_stateTimer = GetExecTimeMs();
			deltaTime = g_stateTimer - g_bootStartTimer;
			if(deltaTime >= 800) {
				if(HAL_EEPROM_IsOk()){
					Event_Post(EVT_SYS_BOOT_EEPROM_STATE, (EventParam_t){.bool_ = true});
				} else {
					Event_Post(EVT_SYS_BOOT_EEPROM_STATE, (EventParam_t){.bool_ = false});
				}
				sprintf(eepromTimestamp, "[%.3f]", (float)deltaTime / 1000);
				Event_Post(EVT_SYS_BOOT_EEPROM_TIMESTAMP, (EventParam_t){.str = eepromTimestamp});
				Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 22});
            	g_systemState = SYS_BOOT_TOUCH_CALIB;
			}
			break;

		case SYS_BOOT_TOUCH_CALIB:
            g_stateTimer = GetExecTimeMs();
			deltaTime = g_stateTimer - g_bootStartTimer;
			if(deltaTime >= 1600) {
				if(GestureEngine_CheckCalib()){
					Event_Post(EVT_SYS_BOOT_TOUCH_STATE, (EventParam_t){.bool_ = true});
				} else {
					Event_Post(EVT_SYS_BOOT_TOUCH_STATE, (EventParam_t){.bool_ = false});
				}

				sprintf(touchTimestamp, "[%.3f]", (float)deltaTime / 1000);
				Event_Post(EVT_SYS_BOOT_TOUCH_TIMESTAMP, (EventParam_t){.str = touchTimestamp});
				Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 34});
            	g_systemState = SYS_BOOT_BATT_STATE;
			}
			break;

		case SYS_BOOT_BATT_STATE:
            g_stateTimer = GetExecTimeMs();
			deltaTime = g_stateTimer - g_bootStartTimer;
			if(deltaTime >= 3200) {
				if(RTC_isBatteryOk()){
					Event_Post(EVT_SYS_BOOT_BATT_STATE, (EventParam_t){.bool_ = true});
				} else {
					Event_Post(EVT_SYS_BOOT_BATT_STATE, (EventParam_t){.bool_ = false});
				}

				sprintf(batteryTimestamp, "[%.3f]", (float)deltaTime / 1000);
				Event_Post(EVT_SYS_BOOT_BATTERY_TIMESTAMP, (EventParam_t){.str = batteryTimestamp});
				Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 46});
            	g_systemState = SYS_BOOT_CHECK_INST;
			}
			break;

		case SYS_BOOT_CHECK_INST:
            g_stateTimer = GetExecTimeMs();
			deltaTime = g_stateTimer - g_bootStartTimer;
	
			if(!g_bWaitingInstHandshake) {
				HAL_CAN_Msg_t msg = (HAL_CAN_Msg_t){
					.id = CAN_ID_ASK_HANDSHAKE,
					.isExtended = false,
					.length = 0,
					.data = NULL,
				};
				HAL_CAN_Transmit(&msg);
				g_bWaitingInstHandshake = true;
			}

			if(g_bInstHandshakeOk) {
				Event_Post(EVT_SYS_BOOT_INST_STATE, (EventParam_t){.bool_ = true});

				sprintf(instTimestamp, "[%.3f]", (float)deltaTime / 1000);
				Event_Post(EVT_SYS_BOOT_INST_TIMESTAMP, (EventParam_t){.str = instTimestamp});
				Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 58});
				ui32LastStateExitTime = GetExecTimeMs();
            	g_systemState = SYS_BOOT_RTC_DATE;
			}
			else if(deltaTime >= 4200) { // Timeout
				Event_Post(EVT_SYS_BOOT_INST_STATE, (EventParam_t){.bool_ = false});

				sprintf(instTimestamp, "[%.3f]", (float)deltaTime / 1000);
				Event_Post(EVT_SYS_BOOT_INST_TIMESTAMP, (EventParam_t){.str = instTimestamp});
				Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 58});
				ui32LastStateExitTime = GetExecTimeMs();
            	g_systemState = SYS_BOOT_RTC_DATE;
			}
			break;

		case SYS_BOOT_RTC_DATE:
            g_stateTimer = GetExecTimeMs();
			deltaTime = g_stateTimer - ui32LastStateExitTime;
			if(deltaTime >= 600) {
				RTC_getFormattedDate(dateStr, sizeof(dateStr));
				Event_Post(EVT_SYS_BOOT_RTC_DATE, (EventParam_t){.str = dateStr});

				sprintf(dateTimestamp, "[%.3f]", (float)(g_stateTimer - g_bootStartTimer) / 1000);
				Event_Post(EVT_SYS_BOOT_DATE_TIMESTAMP, (EventParam_t){.str = dateTimestamp});
				Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 82});
				ui32LastStateExitTime = GetExecTimeMs();
            	g_systemState = SYS_BOOT_RTC_TIME;
			}
			break;

		case SYS_BOOT_RTC_TIME:
            g_stateTimer = GetExecTimeMs();
			deltaTime = g_stateTimer - ui32LastStateExitTime;
			if(deltaTime >= 500) {
			    RTC_getFormattedTime(timeStr, sizeof(timeStr));
				Event_Post(EVT_SYS_BOOT_RTC_TIME, (EventParam_t){.str = timeStr});

				sprintf(timeTimestamp, "[%.3f]", (float)(g_stateTimer - g_bootStartTimer) / 1000);
				Event_Post(EVT_SYS_BOOT_TIME_TIMESTAMP, (EventParam_t){.str = timeTimestamp});
				Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 100});
            	g_systemState = SYS_BOOT_WAIT_5_SECONDS;
            	g_stateTimer = GetExecTimeMs();
			}
			break;
		case SYS_BOOT_WAIT_5_SECONDS:
        {
            uint32_t currentMs = GetExecTimeMs();
            uint32_t elapsedMs = currentMs - g_stateTimer;
			uint32_t rstCntInit = 0;
            
            // Calculate how many whole seconds we've been in this state
            uint32_t secondsElapsed = elapsedMs / 1000;
            
            // Use a static variable to remember the last second we broadcasted.
            // Initialized to an impossible value so it fires immediately at 0s.
            static uint32_t lastReportedSecond = 0xFFFFFFFF;

            // 1. Check if the 5 seconds are completely up (>= 5000 ms)
            if (elapsedMs >= 5000 && g_bIsAutoSeqOn) {
                RTC_getFormattedTime(timeStr, sizeof(timeStr));
                Event_Post(EVT_SYS_BOOT_RTC_TIME, (EventParam_t){.str = timeStr});
                
                // Reset the static tracker in case the system ever soft-reboots
                lastReportedSecond = 0xFFFFFFFF; 
                
				// Restart the rst-to-calib counter
				rstCntInit = 0;
				HAL_EEPROM_writeBytes(EEPROM_GET_ADDRESS(restartCntToCalib), &rstCntInit, sizeof(rstCntInit));

                g_systemState = SYS_SHOW_DASHBOARD_FORM;
            } 
            // 2. Check if the current second is different from the last one we reported
            else if (secondsElapsed != lastReportedSecond && g_bIsAutoSeqOn) {
                lastReportedSecond = secondsElapsed; // Update the tracker
                
                // Calculate countdown: 5, 4, 3, 2, 1
                uint32_t ui32Countdown = 5 - secondsElapsed;
                
                Event_Post(EVT_SYS_BOOT_COUNTDOWN, (EventParam_t){.ui32 = ui32Countdown});
            } else if(!g_bIsAutoSeqOn) {
				if(secondsElapsed != lastReportedSecond) {
					// Restart the rst-to-calib counter
					rstCntInit = 0;
					HAL_EEPROM_writeBytes(EEPROM_GET_ADDRESS(restartCntToCalib), &rstCntInit, sizeof(rstCntInit));

                	lastReportedSecond = secondsElapsed; // Update the tracker
					Event_Post(EVT_UI_BOOT_TOGGLE_CONTINUE_LABEL, (EventParam_t){.ptr = NULL});
				}
			}
            break;
        }
		case SYS_SHOW_DASHBOARD_FORM:
			
			Event_Post(EVT_SYS_SHOW_HOME_FORM, (EventParam_t){.ptr = NULL});
			g_systemState = SYS_IDLE;
			break;
		
		case SYS_CFG_IMPORT_START_TIMEOUT:
		 	ui32ProcStartTime = GetExecTimeMs();	
			g_systemState = SYS_CFG_IMPORT_TIMEOUT;
			break;

		case SYS_CFG_IMPORT_TIMEOUT: {
			if(( GetExecTimeMs() - ui32ProcStartTime ) > CFG_IMPORT_TIMEOUT_MS) {
				Event_Post(EVT_SYS_CFG_IMPORT_TIMEOUT, (EventParam_t){.ptr = NULL});		
				g_systemState = SYS_IDLE;
			}	
			break;
		}

		case SYS_CFG_EXPORT_START_TIMEOUT:
		 	ui32ProcStartTime = GetExecTimeMs();	
			g_systemState = SYS_CFG_EXPORT_TIMEOUT;
			break;

		case SYS_CFG_EXPORT_TIMEOUT: {
			if(( GetExecTimeMs() - ui32ProcStartTime ) > CFG_EXPORT_TIMEOUT_MS) {
				Event_Post(EVT_SYS_CFG_EXPORT_TIMEOUT, (EventParam_t){.ptr = NULL});		
				g_systemState = SYS_IDLE;
			}	
			break;
		}
		case SYS_TEST_INIT: {
            char dateStrLog[12] = {0};
            char timeStrLog[12] = {0};
            
            RTC_getFileFormattedDate(dateStrLog, sizeof(dateStrLog));
            RTC_getFileFormattedTime(timeStrLog, sizeof(timeStrLog));

            // 1. Limpiar e inicializar la estructura de resumen
            memset(&g_testSummary, 0, sizeof(LogTestSummary_t));
            g_testSummary.testType = g_ui32CurrentTestType;
            strncpy(g_testSummary.dateStart, dateStrLog, sizeof(g_testSummary.dateStart) - 1);
            strncpy(g_testSummary.timeStart, timeStrLog, sizeof(g_testSummary.timeStart) - 1);

            float f32TargetValue = 0.0f;
            float f32PresetVoltage = 0.0f;
            bool bIsHighRes = false;
            memset(szCurrentLogFileName, 0, sizeof(szCurrentLogFileName));

            // 2. Extraer parámetros específicos para la potencia y para el resumen
            switch(g_ui32CurrentTestType) {
                case UL_TEST_FAULT: {
                    ul_fault_current_test_s faultCfg = ExperimentCfg_getCurrFaultCfg();
                    ui32TestDurationMs = faultCfg.ui16Duration * 1000;
                    f32TargetValue = faultCfg.f32TargetCurrent;
                    f32PresetVoltage = faultCfg.f32PresetVoltage;
                    bIsHighRes = faultCfg.bIsHighResistence;
                    
                    // Poblamos el resumen
                    g_testSummary.setDurationSec = faultCfg.ui16Duration;
                    g_testSummary.targetCurrent = faultCfg.f32TargetCurrent;
                    g_testSummary.presetVoltage = faultCfg.f32PresetVoltage;
                    g_testSummary.isHighResistance = faultCfg.bIsHighResistence;

                    snprintf(szCurrentLogFileName, sizeof(szCurrentLogFileName), "1:/LOGS/FLT_%s_%s.CSV", dateStrLog, timeStrLog);
                    break;
                }
				case UL_TEST_CRUSH: {
                    ul_crush_test_s crushCfg = ExperimentCfg_getCurrCrushCfg();
                    ui32TestDurationMs = crushCfg.ui16Duration * 1000;
                    f32TargetValue = crushCfg.f32TargetTemp;
                    bIsHighRes = crushCfg.bIsHighResistence;

                    // --- NUEVO: Guardar meta térmica ---
                    g_f32TargetTemp = crushCfg.f32TargetTemp;
                    g_bTargetReached = false;

                    // Poblamos el resumen
                    g_testSummary.setDurationSec = crushCfg.ui16Duration;
                    g_testSummary.targetTemp = crushCfg.f32TargetTemp;
                    g_testSummary.isHighResistance = crushCfg.bIsHighResistence;

                    snprintf(szCurrentLogFileName, sizeof(szCurrentLogFileName), "1:/LOGS/CRSH_%s_%s.CSV", dateStrLog, timeStrLog);
                    break;
                }
				case UL_TEST_SEQUENCE: {
                    const ul_sequence_profile_t* profile = ExperimentCfg_getCurrSequenceCfg();
                    
                    // Ya no usamos 0xFFFFFFFF. Vamos a sumar el tiempo de todos los segmentos.
                    uint32_t totalDurationSec = 0; 
                    float maxCurrent = 0.0f;
								   
                    uint16_t i = 0;
                    for(; i < profile->numSegments; i++) {
                        totalDurationSec += profile->segments[i].durationSec;
                        
                        if(profile->segments[i].targetCurrent > maxCurrent) {
                            maxCurrent = profile->segments[i].targetCurrent;
                        }
                    }
                    
                    ui32TestDurationMs = totalDurationSec * 1000; // Guardamos la duración total real
                    f32TargetValue = 0.0f;
                    bIsHighRes = profile->bIsHighResistence;

                    snprintf(szCurrentLogFileName, sizeof(szCurrentLogFileName), "1:/LOGS/PROF_%s_%s.CSV", dateStrLog, timeStrLog);

                    // Poblamos el resumen
                    g_testSummary.isHighResistance = profile->bIsHighResistence;
                    g_testSummary.numSegments = profile->numSegments;
                    strncpy(g_testSummary.profileFilename, profile->absoluteFilePath, sizeof(g_testSummary.profileFilename) - 1); 
                    g_testSummary.maxCurrentReq = maxCurrent;

                    break;
                }
                default:
                    g_systemState = SYS_IDLE;
                    break;
            }
		
		    // Usamos la variable estática para abrir el log
		    if (LogManager_StartLog(szCurrentLogFileName)) {
		        ExperimentCfg_SendStartCommand(g_ui32CurrentTestType, ui32TestDurationMs / 1000, f32TargetValue, bIsHighRes, f32PresetVoltage);
		        //Event_Post(EVT_SYS_TEST_STARTED, (EventParam_t){.ptr = NULL});
		
		        g_bInstConfirmedStart = false;
		        ui32ProcStartTime = GetExecTimeMs();
		        g_systemState = SYS_TEST_WAIT_CONFIRM; 
		    } else {
		        Event_Post(EVT_SYS_TEST_START_ERROR_FLAG, (EventParam_t){.ptr = NULL});
		        g_systemState = SYS_IDLE; // Si falló al abrir, no hay archivo que borrar
		    }
		    break;
		    }
		case SYS_TEST_WAIT_CONFIRM: {
           	if (g_bInstConfirmedStart) {
        	        g_bInstConfirmedStart = false;
        	        ui32TestStartTime = GetExecTimeMs();
        	        ui32LastLogTime = ui32TestStartTime;

        	        // --- INICIALIZACIÓN DE LA SECUENCIA ---
        	        ui32LastProfileUpdateMs = ui32TestStartTime;
        	        ui16CurrentSegmentIdx = 0;
        	        ui32SegmentStartTimeMs = ui32TestStartTime;
        	        f32SegmentStartCurrent = 0.0f; // Asumimos que la fuente inicia en 0A
        	        f32CurrentProfileSetpoint = 0.0f;
					// Reiniciar el rastreador de actualizaciones de UI para que dispare en t=0
                	ui32LastProgressSec = 0xFFFFFFFF;
        	        g_systemState = SYS_TEST_RUNNING;
        	    }
        	    else if ((GetExecTimeMs() - ui32ProcStartTime) > 6000) {
        	        Event_Post(EVT_SYS_TEST_START_ERROR_FLAG, (EventParam_t){.ptr = NULL});
        	        g_systemState = SYS_TEST_START_FAIL;
        	    }
        	    break;
        	}
		case SYS_TEST_START_FAIL: {
		        // 1. Cerramos el archivo para liberar el *handle* de FatFs
		        LogManager_StopLog();
		
		        // 2. Apagamos la instrumentación por seguridad
		        ExperimentCfg_SendStopCommand();
		
		        // 3. Borramos el archivo .CSV de la memoria USB
		        if (szCurrentLogFileName[0] != '\0') {
		            f_unlink(szCurrentLogFileName);
		            szCurrentLogFileName[0] = '\0'; // Limpiamos la variable
		        }
		
		        g_systemState = SYS_IDLE;
		        break;
		    }
			
		case SYS_TEST_RUNNING: {
            uint32_t currentMs = GetExecTimeMs();
            
            // 1. Tiempo total desde que el usuario dio "Start"
            uint32_t totalElapsedMs = currentMs - ui32TestStartTime;
            
            // 2. Tiempo efectivo para el Ensayo Real
            phaseElapsedMs = 0; 

            // ========================================================
            // LÓGICA DE FASES (CALENTAMIENTO vs HOLD)
            // ========================================================
            if (g_ui32CurrentTestType == UL_TEST_CRUSH) {
                if (!g_bTargetReached) {
                    // El cronómetro de estabilización corre mientras calienta
                    ui32StabilizationTimeMs = totalElapsedMs; 
                    
                    if (g_LatestReadings.tempProbeMain>= g_f32TargetTemp) {
                        g_bTargetReached = true;
                        ui32SegmentStartTimeMs = currentMs; // Reiniciamos el cronómetro principal
                        // Al volverse true, ui32StabilizationTimeMs se congela para siempre en este valor
                    }
                }
                
                if (g_bTargetReached) {
                    phaseElapsedMs = currentMs - ui32SegmentStartTimeMs;
                } else {
                    phaseElapsedMs = 0; // Congelamos el progreso principal en 0
                }
            } else {
                // Para Falla o Secuencia, el tiempo efectivo arranca inmediatamente
                phaseElapsedMs = totalElapsedMs;
                ui32StabilizationTimeMs = 0; 
            }

            // ========================================================
            // ACTUALIZACIÓN DE INTERFAZ GRÁFICA (1 Hz)
            // ========================================================
            // AHORA gatillamos usando el tiempo TOTAL para que la UI se refresque siempre

			uint32_t currentTotalSec = totalElapsedMs / 1000;

            if (currentTotalSec != ui32LastProgressSec) {
                ui32LastProgressSec = currentTotalSec;

                uint32_t remainingSec = 0;
                uint32_t progressPct = 0;

                // 1. Cálculos de la prueba principal
                if (ui32TestDurationMs > 0) {
                    if (ui32TestDurationMs > phaseElapsedMs) {
                        remainingSec = (ui32TestDurationMs - phaseElapsedMs) / 1000;
                    }
                    progressPct = (uint32_t)(((uint64_t)phaseElapsedMs * 100) / ui32TestDurationMs);
                    if (progressPct > 100) progressPct = 100;
                }

                // 2. Formatear Tiempo Restante Principal (Estricto MM:SS)
                uint32_t remMin = remainingSec / 60;
                uint32_t remSec = remainingSec % 60;
                snprintf(szTimeRemaining, sizeof(szTimeRemaining), "%02lu:%02lu", remMin, remSec);

                // 3. Formatear Tiempo de Estabilización (Estricto MM:SS)
                uint32_t stabTotalSec = ui32StabilizationTimeMs / 1000;
                uint32_t stabMin = stabTotalSec / 60;
                uint32_t stabSec = stabTotalSec % 60;
                snprintf(szStabilityTime, sizeof(szStabilityTime), "%02lu:%02lu", stabMin, stabSec);

                // 4. Emitir todos los eventos a la UI
                Event_Post(EVT_UI_TEST_UPDATE_TIME, (EventParam_t){.str = szTimeRemaining});
                Event_Post(EVT_UI_TEST_UPDATE_PROGRESS, (EventParam_t){.ui32 = progressPct});
                Event_Post(EVT_UI_TEST_UPDATE_STABILITY_TIME, (EventParam_t){.str = szStabilityTime});
            }
            
            // ========================================================
            // RECOLECCIÓN DE DATOS (LOGGER)
            // ========================================================
            // logIntervalMs = (g_ui32CurrentTestType == UL_TEST_FAULT) ? 50 : 1000;
			logIntervalMs = 100;
            if ((currentMs - ui32LastLogTime) >= logIntervalMs) {
                ui32LastLogTime = currentMs;
                g_LatestReadings.timeMs = totalElapsedMs;
                LogManager_WriteRow(&g_LatestReadings);
            }
			
			// ========================================================
            // WATCHDOG DE SEGURIDAD (Timeout de 1 Segundo)
            // ========================================================
            // Si la instrumentación no envía el CAN_ID_INST_TEST_FINISHED a tiempo,
            // la HMI toma el control y aborta la prueba por seguridad.
			if (ui32TestDurationMs > 0 && phaseElapsedMs >= (ui32TestDurationMs + 3000)) {
                 g_systemState = SYS_EMERGENCY_STOP;
                 break; // Rompemos el case inmediatamente
            }

            // ========================================================
            // LÓGICA DE CONTROL DE PRUEBAS
            // ========================================================
            if (g_ui32CurrentTestType == UL_TEST_SEQUENCE) {
                if ((currentMs - ui32LastProfileUpdateMs) >= PROFILE_UPDATE_PERIOD_MS) {
                    ui32LastProfileUpdateMs = currentMs;

                    const ul_sequence_profile_t* profile = ExperimentCfg_getCurrSequenceCfg();
                    
                    if (ui16CurrentSegmentIdx < profile->numSegments) {
                        ul_sequence_segment_t seg = profile->segments[ui16CurrentSegmentIdx];
                        uint32_t segDurationMs = seg.durationSec * 1000;
                        uint32_t elapsedInSegMs = currentMs - ui32SegmentStartTimeMs;

                        if (elapsedInSegMs >= segDurationMs) {
                            ui16CurrentSegmentIdx++;
                            ui32SegmentStartTimeMs = currentMs;
                            f32SegmentStartCurrent = f32CurrentProfileSetpoint; 
                            elapsedInSegMs = 0; 
                            
                            if (ui16CurrentSegmentIdx >= profile->numSegments) {
                                g_systemState = SYS_TEST_STOP;
                                break; 
                            }
                            
                            seg = profile->segments[ui16CurrentSegmentIdx];
                            segDurationMs = seg.durationSec * 1000;
                        }

                        switch (seg.transition) {
                            case SEG_STEP:
                                f32CurrentProfileSetpoint = seg.targetCurrent;
                                break;
                            case SEG_HOLD:
                                f32CurrentProfileSetpoint = f32SegmentStartCurrent; 
                                break;
                            case SEG_RAMP:
                                if (segDurationMs > 0) {
                                    float progress = (float)elapsedInSegMs / (float)segDurationMs;
                                    f32CurrentProfileSetpoint = f32SegmentStartCurrent + 
                                        ((seg.targetCurrent - f32SegmentStartCurrent) * progress);
                                }
                                break;
                        }

                        union { float f; uint8_t bytes[4]; } canData;
                        canData.f = f32CurrentProfileSetpoint;
                        
                        HAL_CAN_Msg_t setpointMsg = {
                            .id = CAN_ID_INST_TEST_PROFILE_SETPOINT, 
                            .isExtended = false,
                            .length = 4,
                        };
                        memcpy(setpointMsg.data, canData.bytes, 4);
                        HAL_CAN_Transmit(&setpointMsg);
                    }
                }
            }
            break;
        }

		case SYS_EMERGENCY_STOP: {
            // 1. Obtener los datos finales de tiempo
            RTC_getFileFormattedTime(g_testSummary.timeEnd, sizeof(g_testSummary.timeEnd));
            
            // Calculamos la duración real hasta el momento del fallo
            g_testSummary.realDurationSec = (GetExecTimeMs() - ui32TestStartTime) / 1000;

            // 2. APAGADO FORZADO DE POTENCIA (Mandar el 0x451 inmediatamente)
            ExperimentCfg_SendStopCommand();
            
            // 3. Escribir el resumen final al log 
            // (Opcional: Si tienes una bandera en tu struct, puedes marcar g_testSummary.bErrorTimeout = true;)
            LogManager_WriteSummary(&g_testSummary);

            // 4. Cerrar y asegurar el archivo CSV para no corromper los datos guardados
            LogManager_StopLog();
            
            // 5. Avisarle a la UI que ocurrió una detención de emergencia
            Event_Post(EVT_SYS_TEST_EMERGENCY_STOPPED, (EventParam_t){.str = szCurrentLogFileName});
            Event_Post(EVT_SYS_TEST_FINISHED_FAIL, (EventParam_t){.ptr = NULL});
            
            g_systemState = SYS_IDLE;   
            break;
        }

		case SYS_TEST_STOP: {
            // 1. Obtener los datos finales de tiempo
            RTC_getFileFormattedTime(g_testSummary.timeEnd, sizeof(g_testSummary.timeEnd));
            g_testSummary.realDurationSec = (GetExecTimeMs() - ui32TestStartTime) / 1000;

            // 2. Apagar potencia
            ExperimentCfg_SendStopCommand();
            
            // 3. Escribir el resumen final al log
            LogManager_WriteSummary(&g_testSummary);

            // 4. Cerrar y asegurar el archivo
            LogManager_StopLog();
            
			Event_Post(EVT_SYS_TEST_SAVED_LOG_NAME, (EventParam_t){.str = szCurrentLogFileName});
            Event_Post(EVT_SYS_TEST_FINISHED_OK, (EventParam_t){.ptr = NULL});
            g_systemState = SYS_IDLE;	
            break;
        }

		case SYS_IDLE:
			break;
    }

	// The date and time must be updated independently of which state we are in
	if(GetExecTimeMs() % 1000 == 0) {
		RTC_getFormattedTime(globalTimeStr, sizeof(globalTimeStr));
		Event_Post(EVT_SYS_TIME_CHANGED, (EventParam_t){.str = globalTimeStr});
	}
}
