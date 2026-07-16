#ifndef HAL_INST_CAN_H
#define HAL_INST_CAN_H

#include <stdint.h>
#include <stdbool.h>

// Definición genérica de un mensaje CAN para tu instrumentación
typedef struct {
    uint32_t id;         // Identificador del mensaje (Standard 11-bit o Extended 29-bit)
    uint8_t  data[8];    // Payload de datos
    uint8_t  length;     // Longitud de los datos (0 a 8)
    bool     isExtended; // true si usa ID de 29 bits
} HAL_CAN_Msg_t;

// Firma de la función de callback (El "número de teléfono" a llamar al recibir datos)
typedef void (*HAL_CAN_RxCallback_fn)(HAL_CAN_Msg_t *msg);

// Initialise CAN0 hardware at the given bitrate (e.g. 500000 for 500 kbps)
bool HAL_CAN_Init(uint32_t systemClock, uint32_t bitrate);

// Transmit a message. Returns false if the TX mailbox is still busy.
bool HAL_CAN_Transmit(HAL_CAN_Msg_t *msg);

// Returns true while a transmitted frame has not yet been confirmed sent.
bool HAL_CAN_IsTxBusy(void);

// Register the function called (in ISR context) when a frame arrives.
void HAL_CAN_SetRxCallback(HAL_CAN_RxCallback_fn callback);

#endif // HAL_INST_CAN_H


