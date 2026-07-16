#ifndef HAL_USB_H_
#define HAL_USB_H_

#include <stdbool.h>
#include <stdint.h>

// =====================================================================
// Inicialización del Hardware USB (Pines y Host Controller)
// =====================================================================
bool HAL_USB_Init(void);

// =====================================================================
// Tarea del Estado de Máquina USB (Debe ir en tu bucle principal)
// =====================================================================
void HAL_USB_Task(void);

// =====================================================================
// Retorna true si una memoria USB está insertada y lista para leer/escribir
// =====================================================================
bool HAL_USB_IsReady(void);

#endif // HAL_USB_H_


