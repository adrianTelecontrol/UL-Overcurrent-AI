#ifndef DATA_MODEL_H
#define DATA_MODEL_H

#include <stdint.h>
#include "log_manager.h" // Para usar LogDataRow_t

// Instancia global con los últimos valores conocidos de los sensores
// (El SysManager o TestEngine leerá esta estructura para guardar los logs)
extern LogDataRow_t g_LatestReadings;

/**
 * @brief Inicializa el modelo de datos suscribiendo las funciones internas
 * a los eventos CAN correspondientes de la instrumentación.
 */
void DataModel_Init(void);

#endif // DATA_MODEL_H