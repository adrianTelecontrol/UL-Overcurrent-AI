

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/sysctl.h"

#include <string.h>
#include "cfg_manager.h"
#include "event_engine.h"

#include <fatfs/src/ff.h>

#include "can_id_map.h"
#include "helpers.h"

#include "hal_inst_can.h"

#define CFG_MAGIC_WORD 0xECC4 // 60612d


// =========================================================================
// FUNCIÓN PARA ENVIAR EL ARCHIVO DE CONFIGURACIÓN A LA INSTRUMENTACIÓN
// =========================================================================
bool CfgManager_SendConfigFileToInst(const char *filePath) {
    FIL fp;
    FRESULT res;
    UINT bytesRead;
    uint16_t magicWord; // Un solo entero de 16 bits (2 bytes)
	uint32_t countBusy = 0;

    // 1. Abrir archivo
    res = f_open(&fp, filePath, FA_READ);
    if (res != FR_OK) return false;

    // 2. Leer y validar la palabra mágica (2 bytes)
    f_read(&fp, &magicWord, 2, &bytesRead);
    if (bytesRead != 2 || magicWord != CFG_MAGIC_WORD) {
        f_close(&fp);
        return false; // El archivo no es un archivo de configuración válido
    }

    // 3. Obtener el tamaño total del archivo
    uint32_t totalSize32 = f_size(&fp);
    
    // Tu protocolo dice: "Cantidad en bytes, entero 2 bytes".
    if (totalSize32 > 0xFFFF) { 
        f_close(&fp); 
        return false; // Archivo demasiado grande para el protocolo
    }
    uint16_t totalSize = (uint16_t)totalSize32;

    // 4. Regresar el cursor al inicio para transmitir el archivo completo
    f_lseek(&fp, 0); 

    HAL_CAN_Msg_t txMsg;
    
    // ======================================================
    // FASE A: ENVIAR BANDERA START
    // ======================================================
    txMsg.id = CAN_ID_START_CFG_DATA;
    txMsg.isExtended = false;
    txMsg.length = 2; // Dos bytes para el tamaño
    memcpy(txMsg.data, &totalSize, 2);
    while(!HAL_CAN_Transmit(&txMsg)); 

    // ======================================================
    // FASE B: ENVIAR BYTES (Carga útil)
    // ======================================================
    uint16_t checksum = 0;
    uint8_t buffer[8];
    uint16_t remainingBytes = totalSize;

    while (remainingBytes > 0) {
        UINT toRead = (remainingBytes > 8) ? 8 : remainingBytes;
		if(remainingBytes < 10) {
			SysCtlDelay(10);
		}
        f_read(&fp, buffer, toRead, &bytesRead);
        
        txMsg.id = CAN_ID_CFG_DATA_FRAME;
        txMsg.length = bytesRead;
        memcpy(txMsg.data, buffer, bytesRead);
        
        // ¡Bloqueante internamente si los buffers CAN TX están llenos!
        while(!HAL_CAN_Transmit(&txMsg));
		countBusy++;
        
        remainingBytes -= bytesRead;

		SysCtlDelay(MS_2_CLK(1));
    }

	Event_Post(EVT_CAN_INST_CURRENT_PRIMARY, (EventParam_t){.ui32 = countBusy});
	SysCtlDelay(MS_2_CLK(50));
	
    // ======================================================
    // FASE C: ENVIAR BANDERA END (Checksum)
    // ======================================================
    txMsg.id = CAN_ID_CFG_DATA_END;
    txMsg.length = 2; // Dos bytes para el checksum
    memcpy(txMsg.data, &checksum, 2);
    HAL_CAN_Transmit(&txMsg);

    // Limpieza
    f_close(&fp);
    return true;
}


void CfgManager_RequestConfigFromInst(void) {
    HAL_CAN_Msg_t txMsg;
    txMsg.id = CAN_ID_HMI_ASK_EXPORT_CFG; // ID para pedirle el archivo a la inst.
    txMsg.isExtended = false;
    txMsg.length = 0; 
    
    HAL_CAN_Transmit(&txMsg);
}