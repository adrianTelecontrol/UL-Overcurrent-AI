#ifndef GESTURE_ENGINE_H_
#define GESTURE_ENGINE_H_

#define EEPROM_CALIBRATED_MAGIC_NUMBER	0xFB82

typedef enum
{
    GESTURE_EMPTY,
	GESTURE_PRESSED,
    GESTURE_RELEASE,
    GESTURE_DRAG,
    GESTURE_LOCK_OBJ,
} gesture_type_e;

void GestureEngine_Init(void);

void GestureEngine_Task(void);

void GestureEngine_CalibrateScreen(void);

bool GestureEngine_CheckCalib(void);

#endif 	// GESTURE_ENGINE_H_


