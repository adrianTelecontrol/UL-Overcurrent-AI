#ifndef ADJUST_NUMPAD_FORM_H_ 
#define ADJUST_NUMPAD_FORM_H_

#include <stdint.h>

extern int16_t g_i16AdjustNumpadIndex;

// List of values that can be modified
typedef enum {
	ADJ_VOLTAGE_PRIMARY = 0,
 	ADJ_VOLTAGE_SECONDARY,	
 	ADJ_CURRENT_PRIMARY,
	ADJ_CURRENT_SECONDARY,	
	ADJ_TEMP_PROBE_MAIN,
	ADJ_TEMP_PROBE_SECONDARY,
	ADJ_TEMP_TX_PRIMARY,
	ADJ_TEMP_TX_SECONDARY,
	ADJ_TEMP_CABLE_A,
	ADJ_TEMP_CABLE_B,
	ADJ_TEMP_CJC,
	ADJ_VARIAC_VOLTAGE,
} adj_value_e;

void initAdjustNumpadForm(void);


#endif // ADJUST_NUMPAD_FORM_H_