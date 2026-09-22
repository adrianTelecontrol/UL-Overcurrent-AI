
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "driverlib/interrupt.h"

#include "hal_inst_can.h"
#include "can_id_map.h"
#include "event_engine.h"
#include "experiments_cfg.h"
#include "helpers.h" 
#include "rtc_module.h"
#include "file_manager.h"

#include "inst_can_buffer.h"

#include <fatfs/src/ff.h>

#define CAN_TX_QUEUE_SIZE 8U

#if INST_TEMP_PID_ENABLE_INTEGRAL
#define INST_TEMP_PID_APPLY_MESSAGE_COUNT 4U
#else
#define INST_TEMP_PID_APPLY_MESSAGE_COUNT 3U
#endif

static FIL g_cfgExportFile;
static bool g_bIsReceivingCfg = false;
static uint16_t g_ui16ExpectedCfgSize = 0;
static uint16_t g_ui16ReceivedBytes = 0;
static uint16_t g_ui16CalculatedChecksum = 0;
static char g_szLastExportPath[64]; // Para guardar la ruta y mandarla a la UI
static char g_szInstStatus[20] = "LISTO";

static HAL_CAN_Msg_t g_CanRxQueue[CAN_RX_BUFFER_SIZE];
static volatile uint16_t g_ui16Head = 0;
static volatile uint16_t g_ui16Tail = 0;
static HAL_CAN_Msg_t g_CanTxQueue[CAN_TX_QUEUE_SIZE];
static uint8_t g_ui8TxHead = 0U;
static uint8_t g_ui8TxTail = 0U;

static uint8_t InstManager_TxQueueFree(void) {
    return (uint8_t)((g_ui8TxTail - g_ui8TxHead - 1U) &
                     (CAN_TX_QUEUE_SIZE - 1U));
}

static bool InstManager_QueueTx(const HAL_CAN_Msg_t *msg) {
    uint8_t nextHead;

    if (msg == NULL || msg->length > 8U) {
        return false;
    }

    nextHead = (uint8_t)((g_ui8TxHead + 1U) & (CAN_TX_QUEUE_SIZE - 1U));
    if (nextHead == g_ui8TxTail) {
        return false;
    }

    g_CanTxQueue[g_ui8TxHead] = *msg;
    g_ui8TxHead = nextHead;
    return true;
}

static void InstManager_ProcessTxQueue(void) {
    if (g_ui8TxTail == g_ui8TxHead || HAL_CAN_IsTxBusy()) {
        return;
    }

    if (HAL_CAN_Transmit(&g_CanTxQueue[g_ui8TxTail])) {
        g_ui8TxTail = (uint8_t)((g_ui8TxTail + 1U) &
                                (CAN_TX_QUEUE_SIZE - 1U));
    }
}

static HAL_CAN_Msg_t InstManager_CreateFloatMessage(uint32_t id, float value) {
    HAL_CAN_Msg_t msg = {0};
    msg.id = id;
    msg.length = sizeof(float);
    msg.isExtended = false;
    memcpy(msg.data, &value, sizeof(float));
    return msg;
}

void InstCanBuffer_Init(void) {
    g_ui16Head = 0;
    g_ui16Tail = 0;
    g_ui8TxHead = 0U;
    g_ui8TxTail = 0U;
}

bool InstManager_RequestTempPidValues(void) {
    HAL_CAN_Msg_t msg = {0};
    msg.id = CAN_ID_REQ_TEMP_PID_VALUES;
    return InstManager_QueueTx(&msg);
}

bool InstManager_ApplyTempPidValues(const InstTempPidValues_t *values) {
    HAL_CAN_Msg_t msg;

    if (values == NULL ||
        InstManager_TxQueueFree() < INST_TEMP_PID_APPLY_MESSAGE_COUNT) {
        return false;
    }

    msg = InstManager_CreateFloatMessage(CAN_ID_REQ_TEMP_PID_SET_KP,
                                         values->kp);
    InstManager_QueueTx(&msg);
#if INST_TEMP_PID_ENABLE_INTEGRAL
    msg = InstManager_CreateFloatMessage(CAN_ID_REQ_TEMP_PID_SET_KI,
                                         values->ki);
    InstManager_QueueTx(&msg);
#endif
    msg = InstManager_CreateFloatMessage(CAN_ID_REQ_TEMP_PID_SET_KD,
                                         values->kd);
    InstManager_QueueTx(&msg);

    memset(&msg, 0, sizeof(msg));
    msg.id = CAN_ID_REQ_TEMP_PID_APPLY;
    InstManager_QueueTx(&msg);
    return true;
}

bool InstManager_RestoreTempPidDefaults(void) {
    HAL_CAN_Msg_t msg = {0};
    msg.id = CAN_ID_REQ_TEMP_PID_RESTORE_DEFAULTS;
    return InstManager_QueueTx(&msg);
}

bool InstCanBuffer_Push(const HAL_CAN_Msg_t *msg) {
    uint16_t nextHead = (g_ui16Head + 1) % CAN_RX_BUFFER_SIZE;
    
    // Si el búfer está lleno, descartamos el mensaje más viejo (u omitimos, según diseño)
    if (nextHead == g_ui16Tail) {
        return false; // Búfer Overflow
    }

    g_CanRxQueue[g_ui16Head] = *msg;
    g_ui16Head = nextHead;
    
    return true;
}

bool InstCanBuffer_Pop(HAL_CAN_Msg_t *outMsg) {
    // Si head == tail, el búfer está vacío
    if (g_ui16Head == g_ui16Tail) {
        return false;
    }

    // Deshabilitar interrupciones globalmente por 1 microsegundo para evitar 
    // condiciones de carrera si la ISR modifica Tail/Head mientras leemos.
    bool bIntsEnabled = IntMasterDisable();
    
    *outMsg = g_CanRxQueue[g_ui16Tail];
    g_ui16Tail = (g_ui16Tail + 1) % CAN_RX_BUFFER_SIZE;
    
    // Restaurar interrupciones a su estado anterior
    if(!bIntsEnabled) {
        IntMasterEnable();
    }

    return true;
}
	

// Variables estáticas para mantener el estado
static uint32_t g_ui32LastMessageTime = 0;
static uint32_t g_ui32LastHandshakeTick = 0; // Temporizador para reintentos
static bool g_bIsInstrumentSynced = false;
static bool g_bReportedInstrumentSynced = false;

static void InstManager_PublishSyncState(void) {
    EventID_e event;

    if (g_bReportedInstrumentSynced == g_bIsInstrumentSynced) {
        return;
    }

    event = g_bIsInstrumentSynced ? EVT_SYS_INST_SYNC_RESTORED
                                 : EVT_SYS_INST_UNSYNC;
    /* A full event queue must not permanently lose a state transition. */
    if (Event_Post(event, (EventParam_t){.ptr = NULL})) {
        g_bReportedInstrumentSynced = g_bIsInstrumentSynced;
    }
}

void InstManager_Task(void) {
    HAL_CAN_Msg_t msg;
    bool bReceivedInThisCycle = false;
    uint32_t currentMs = GetExecTimeMs();

    /* Retry pending state before a new telemetry batch can fill the queue. */
    InstManager_PublishSyncState();
    
    // 1. Procesar todos los mensajes entrantes (Extracción Rápida)
    while (InstCanBuffer_Pop(&msg)) {
        bReceivedInThisCycle = true;
        float fVal;
        
        switch (msg.id) {
            case CAN_ID_INST_CURRENT_PRIMARY:
                memcpy(&fVal, msg.data, sizeof(float));
                Event_Post(EVT_CAN_INST_CURRENT_PRIMARY, (EventParam_t){.f32 = fVal});
                break;
                
            case CAN_ID_INST_CURRENT_SECUNDARY:
                memcpy(&fVal, msg.data, sizeof(float));
                Event_Post(EVT_CAN_INST_CURRENT_SECUNDARY, (EventParam_t){.f32 = fVal});
                break;

            case CAN_ID_INST_VOLTAGE_PRIMARY:
                memcpy(&fVal, msg.data, sizeof(float));
                Event_Post(EVT_CAN_INST_VOLTAGE_PRIMARY, (EventParam_t){.f32 = fVal});
                break;

            case CAN_ID_INST_VOLTAGE_SECONDARY:
                memcpy(&fVal, msg.data, sizeof(float));
                Event_Post(EVT_CAN_INST_VOLTAGE_SECONDARY, (EventParam_t){.f32 = fVal});
                break;
                
            case CAN_ID_INST_TEMP_TX_PRIMARY:
                memcpy(&fVal, msg.data, sizeof(float));
                Event_Post(EVT_CAN_INST_TEMP_TX_PRIMARY, (EventParam_t){.f32 = fVal});
				Event_Post(EVT_CAN_INST_TC_TX_PRIMARY, (EventParam_t){.ui32 = msg.data[4]});
                break;

            case CAN_ID_INST_TEMP_TX_SECONDARY:
                memcpy(&fVal, msg.data, sizeof(float));
                Event_Post(EVT_CAN_INST_TEMP_TX_SECONDARY, (EventParam_t){.f32 = fVal}); 
				Event_Post(EVT_CAN_INST_TC_TX_SECONDARY, (EventParam_t){.ui32 = msg.data[4]});
                break;

            case CAN_ID_INST_TEMP_PROBE_MAIN:
                memcpy(&fVal, msg.data, sizeof(float));
                Event_Post(EVT_CAN_INST_TEMP_PROBE_MAIN, (EventParam_t){.f32 = fVal}); 
				Event_Post(EVT_CAN_INST_TC_PROBE_MAIN, (EventParam_t){.ui32 = msg.data[4]});
                break;

            case CAN_ID_INST_TEMP_PROBE_SECONDARY:
                memcpy(&fVal, msg.data, sizeof(float));
                Event_Post(EVT_CAN_INST_TEMP_PROBE_SECONDARY, (EventParam_t){.f32 = fVal}); 
				Event_Post(EVT_CAN_INST_TC_PROBE_SECONDARY, (EventParam_t){.ui32 = msg.data[4]});
                break;

            case CAN_ID_INST_TEMP_CABLE_A:
                memcpy(&fVal, msg.data, sizeof(float));
                Event_Post(EVT_CAN_INST_TEMP_CABLE_A, (EventParam_t){.f32 = fVal}); 
                Event_Post(EVT_CAN_INST_TC_CABLE_A, (EventParam_t){.ui32 = msg.data[4]}); 
                break;

            case CAN_ID_INST_TEMP_CABLE_B:
                memcpy(&fVal, msg.data, sizeof(float));
                Event_Post(EVT_CAN_INST_TEMP_CABLE_B, (EventParam_t){.f32 = fVal}); 
                Event_Post(EVT_CAN_INST_TC_CABLE_B, (EventParam_t){.ui32 = msg.data[4]}); 
                break;

            case CAN_ID_INST_TEMP_CJC:
                memcpy(&fVal, msg.data, sizeof(float));
                Event_Post(EVT_CAN_INST_TEMP_CJC, (EventParam_t){.f32 = fVal}); 
                Event_Post(EVT_CAN_INST_TC_CJC, (EventParam_t){.ui32 = msg.data[4]}); 
                break;

			case CAN_ID_INST_STATUS: {
				if(msg.length == 0U) break;
				
				test_manager_state_t state = (test_manager_state_t)msg.data[0];
				if(state == TEST_STATE_IDLE) 
					strcpy(g_szInstStatus, "DESARMADO");
				else if(state == TEST_STATE_ABORTED)
					strcpy(g_szInstStatus, "ABORTADO");
				else if(state == TEST_STATE_FINISHED)
					strcpy(g_szInstStatus, "FINALIZADO");
				else if(state == TEST_STATE_RUNNING)
					strcpy(g_szInstStatus, "EJECUTANDO");

				Event_Post(EVT_CAN_INST_STATE, ( EventParam_t ){.str = g_szInstStatus});
				break;
			}
            case CAN_ID_INST_HANDSHAKE_OK:
                Event_Post(EVT_SYS_BOOT_HANDSHAKE_OK, (EventParam_t){.ptr = NULL});
                break;

            case CAN_ID_INST_SEND_CONFIG_FILE_START: {
				if(g_bIsReceivingCfg || msg.length < 2U) break;
                memcpy(&g_ui16ExpectedCfgSize, msg.data, 2);
                g_ui16ReceivedBytes = 0;
                g_ui16CalculatedChecksum = 0;
                
                // Crear el archivo. NOTA: Aquí puedes usar tu RTC para crear nombres dinámicos.
                // Por simplicidad, usaremos un nombre estático que se sobreescribe.
				char timeStr[11] = {0};
				char dateStr[11] = {0};
				RTC_getFileFormattedDate(dateStr, sizeof(dateStr));
				RTC_getFileFormattedTime(timeStr, sizeof(timeStr));
                snprintf(g_szLastExportPath, sizeof(g_szLastExportPath), "%s/%s_%s.TEL", FM_getDriveString(DRIVE_USB_ID), dateStr, timeStr);
                
                if (f_open(&g_cfgExportFile, g_szLastExportPath, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
                    g_bIsReceivingCfg = true;
                } else {
                    // Si la USB no está conectada o falla la apertura
                    Event_Post(EVT_SYS_CFG_EXPORT_ERROR, (EventParam_t){.ptr = NULL});
                }
                break;
			}

            case CAN_ID_INST_SEND_CONFIG_FILE_BYTE:
                if (g_bIsReceivingCfg) {
                    UINT bytesWritten;
                    f_write(&g_cfgExportFile, msg.data, msg.length, &bytesWritten);
                    
                    g_ui16ReceivedBytes += msg.length;
                    int i = 0;
                    for (; i < msg.length; i++) {
                        g_ui16CalculatedChecksum += msg.data[i];
                    }
                }
                break;

            case CAN_ID_INST_SEND_CONFIG_FILE_END:
                if (g_bIsReceivingCfg) {
                    uint16_t receivedChecksum;
                    memcpy(&receivedChecksum, msg.data, 2);
                    
                    f_close(&g_cfgExportFile);
                    g_bIsReceivingCfg = false;

                    // Verificar integridad del archivo recibido
                    if (receivedChecksum == g_ui16CalculatedChecksum && g_ui16ReceivedBytes == g_ui16ExpectedCfgSize) {
                        // Éxito: Pasamos la ruta del archivo a la UI mediante el puntero
                        Event_Post(EVT_SYS_CFG_EXPORT_SUCCESS, (EventParam_t){.ptr = (void*)g_szLastExportPath});
                    } else {
                        // Archivo corrupto o incompleto
                        f_unlink(g_szLastExportPath); // Borramos el archivo malo
                        Event_Post(EVT_SYS_CFG_EXPORT_ERROR, (EventParam_t){.ptr = NULL});
                    }
                }
                break;
			case CAN_ID_INST_ERROR_CFGFILE_SIZE:
				Event_Post(EVT_CAN_INST_CFG_LOAD_ERROR, (EventParam_t){.ui32 = CAN_ID_INST_ERROR_CFGFILE_SIZE});
				break;
			case CAN_ID_INST_ERROR_CFGFILE_CHKSUM:
				Event_Post(EVT_CAN_INST_CFG_LOAD_ERROR, (EventParam_t){.ui32 = CAN_ID_INST_ERROR_CFGFILE_CHKSUM});
				break;
			case CAN_ID_INST_ERROR_CFGFILE_SEND_TIMEOUT:
				Event_Post(EVT_CAN_INST_CFG_LOAD_ERROR, (EventParam_t){.ui32 = CAN_ID_INST_ERROR_CFGFILE_SEND_TIMEOUT});
				break;
			case CAN_ID_INST_INFO_DOWN_CFG_FILE_OK:
				Event_Post(EVT_CAN_INST_CFG_LOAD_OK, (EventParam_t){.ptr = NULL});
				break;
            case CAN_ID_INST_DIN_CHANGED:
                // msg.data[0] = Canal (0-9)
                // msg.data[1] = Estado (0 o 1)
                Event_Post(EVT_CAN_INST_DIN_CHANGED, (EventParam_t){
                    .ui32 = (msg.data[0] << 16) | msg.data[1]
                });
                break;
            case CAN_ID_INST_DOUT_CHANGED:
                // msg.data[0] = Canal (0-9)
                // msg.data[1] = Estado (0 o 1)
                Event_Post(EVT_CAN_INST_DOUT_CHANGED, (EventParam_t){
                    .ui32 = (msg.data[0] << 16) | msg.data[1]
                });
                break;
            case CAN_ID_INST_VARIAC_VOLTAGE:
                // 1. Extraer y enviar el Voltaje (Bytes 0 a 3)
                memcpy(&fVal, msg.data, sizeof(float));
                Event_Post(EVT_CAN_INST_VARIAC_VOLTAGE, (EventParam_t){.f32 = fVal});
                
                // 2. Extraer y enviar el Estado/Flags (Byte 4)
                // 0x01 - LOW, 0x02 - HIGH, 0x04 - ALARM
                Event_Post(EVT_CAN_INST_VARIAC_STATUS, (EventParam_t){.ui32 = msg.data[4]}); 
                break;
			case CAN_ID_INST_TEST_START_OK:
				Event_Post(EVT_CAN_INST_TEST_START_OK, (EventParam_t){.ptr = NULL});
			break;
			case CAN_ID_INST_TEST_START_FAIL:
				Event_Post(EVT_CAN_INST_TEST_START_FAIL, (EventParam_t){.ptr = NULL});
			break;
			case CAN_ID_INST_TEST_FINISHED:
				Event_Post(EVT_CAN_INST_TEST_FINISHED, (EventParam_t){.ptr = NULL});
			break;
			case CAN_ID_ACK_FACTORY_RESET:
				Event_Post(EVT_CAN_INST_ACK_FACTORY_RESET, ( EventParam_t ){.bool_ = msg.data[0]});
			break;
		case CAN_ID_ACK_SAVE_EEPROM:
				Event_Post(EVT_CAN_INST_ACK_SAVE_EEPROM, ( EventParam_t ){.bool_ = msg.data[0]});
			break;
			case CAN_ID_INST_TEMP_PID_KP:
				if (msg.length >= sizeof(float)) {
					memcpy(&fVal, msg.data, sizeof(float));
					Event_Post(EVT_CAN_INST_TEMP_PID_KP, (EventParam_t){.f32 = fVal});
				}
			break;
			case CAN_ID_INST_TEMP_PID_KI:
				if (msg.length >= sizeof(float)) {
					memcpy(&fVal, msg.data, sizeof(float));
					Event_Post(EVT_CAN_INST_TEMP_PID_KI, (EventParam_t){.f32 = fVal});
				}
			break;
			case CAN_ID_INST_TEMP_PID_KD:
				if (msg.length >= sizeof(float)) {
					memcpy(&fVal, msg.data, sizeof(float));
					Event_Post(EVT_CAN_INST_TEMP_PID_KD, (EventParam_t){.f32 = fVal});
				}
			break;
			case CAN_ID_INST_TEMP_PID_APPLY_ACK:
				Event_Post(EVT_CAN_INST_TEMP_PID_APPLY_ACK,
				           (EventParam_t){.bool_ = msg.length > 0U && msg.data[0] != 0U});
			break;
			case CAN_ID_INST_TEMP_PID_DEFAULTS_ACK:
				Event_Post(EVT_CAN_INST_TEMP_PID_DEFAULTS_ACK,
				           (EventParam_t){.bool_ = msg.length > 0U && msg.data[0] != 0U});
			break;
            default:
                break;
        }
    }

    // 2. Lógica del Watchdog de Recepción (Desconexión)
    if (bReceivedInThisCycle) {
        g_ui32LastMessageTime = currentMs;
        
        if (!g_bIsInstrumentSynced) {
            g_bIsInstrumentSynced = true;
        }
    } else {
        if (g_bIsInstrumentSynced) {
            if ((currentMs - g_ui32LastMessageTime) >= 5000) {
                g_bIsInstrumentSynced = false; 
            }
        }
    }

    InstManager_PublishSyncState();

    // 3. Lógica de Auto-Recuperación (Transmisión del Handshake)
    if (!g_bIsInstrumentSynced) {
        // Intentar contactar a la instrumentación cada 1 segundo (1000 ms)
        if ((currentMs - g_ui32LastHandshakeTick) >= 1000) {
            g_ui32LastHandshakeTick = currentMs;

            // Preparar el mensaje CAN
            HAL_CAN_Msg_t txMsg;
            txMsg.id = CAN_ID_ASK_HANDSHAKE; // 0x200
            txMsg.isExtended = false;             // ID estándar de 11 bits
            txMsg.length = 0;                     // Cero bytes de datos

            // Enviarlo al bus usando la capa HAL que construiste
            HAL_CAN_Transmit(&txMsg);
        }
    }

    InstManager_ProcessTxQueue();
}



