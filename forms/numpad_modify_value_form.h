#ifndef NUMPAD_MODIFY_VALUE_FORM_H_
#define NUMPAD_MODIFY_VALUE_FORM_H_

#include <stdint.h>

extern int16_t g_i16NumpadModifyValueIndex;

// List of values that can be modified
#define FAULT_TEST_CONFIG_DURATION	0
#define FAULT_TEST_CONFIG_CURRENT	1

void initNumpadModifyValueForm(void);


#endif // FAULT_TEST_CONFIG_FORM_H_