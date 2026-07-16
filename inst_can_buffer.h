#ifndef INST_CAN_BUFFER_H
#define INST_CAN_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_inst_can.h"

#define CAN_RX_BUFFER_SIZE 64 // Suficiente para ráfagas rápidas

// Inicializa el búfer
void InstCanBuffer_Init(void);

// Llamado por el callback del HAL_CAN cuando llega un mensaje (Contexto ISR)
bool InstCanBuffer_Push(const HAL_CAN_Msg_t *msg);

// Llamado por tu bucle principal para extraer mensajes (Contexto Tarea)
bool InstCanBuffer_Pop(HAL_CAN_Msg_t *outMsg);

void InstManager_Task(void);

#endif