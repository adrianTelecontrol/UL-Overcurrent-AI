#ifndef EEPROM_MAP_H_
#define EEPROM_MAP_H_

#include <stdint.h>
#include <stddef.h> 

#define EEPROM_START_ADDR   0x000
#define EEPROM_END_ADDR     0x1800 


// EEPROM Layout (map)
typedef struct {
    uint32_t isCalibrated;           // 4 bytes
    uint32_t touchTransform[6];      // 24 bytes (Matriz de calibración de la pantalla)
    
    uint32_t brightnesLevel;         // 4 bytes
	uint32_t startupTimes;			 // 4 bytes
	
	uint32_t restartCntToCalib;		// 4 byte
} EEPROM_Layout_t;

// Test at compile time if the map size has exceed the size of the EEPROM
_Static_assert(sizeof(EEPROM_Layout_t) <= EEPROM_END_ADDR, "EEPROM overflow!");

#define EEPROM_GET_ADDRESS(f) (EEPROM_START_ADDR + offsetof(EEPROM_Layout_t, f))

#endif // EEPROM_MAP_H_



