#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <inc/hw_eeprom.h>
#include <driverlib/sysctl.h>
#include <driverlib/eeprom.h>

#include "hal_eeprom.h"

bool HAL_EEPROM_init(void) {
    if(!SysCtlPeripheralReady(SYSCTL_PERIPH_EEPROM0)) {
        SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
        while(!SysCtlPeripheralReady(SYSCTL_PERIPH_EEPROM0));
    }
    
    // EEPROMInit performs a recovery if power was lost during a previous write
    if(EEPROMInit() != EEPROM_INIT_OK) {
        return false;
    }

    return true; 
}

bool HAL_EEPROM_writeBytes(uint32_t addr, const uint32_t *data, uint32_t size) {
    if(data == NULL || size == 0) return false;

    // TM4C Hardware Guard: Address and Size must be multiples of 4 bytes
    if((addr % 4 != 0) || (size % 4 != 0)) {
        return false; 
    }

    // EEPROMProgram returns 0 on success, or an error code
    uint32_t status = EEPROMProgram((uint32_t *)data, addr, size);
    
    return (status == 0);
}

bool HAL_EEPROM_readBytes(uint32_t addr, uint32_t *data, uint32_t size) {
    if(data == NULL || size == 0) return false;

    // TM4C Hardware Guard: Address and Size must be multiples of 4 bytes
    if((addr % 4 != 0) || (size % 4 != 0)) {
        return false; 
    }

    EEPROMRead(data, addr, size);
    
    return true;
}

bool HAL_EEPROM_IsOk(void) {
	if(EEPROMStatusGet() == 0) { 
		return true;
	} else {
		return false;	
	}
}

