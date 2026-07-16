
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "driverlib/sysctl.h"

#include "rtc_module.h"
#include "event_engine.h"
#include "helpers.h"
#include "file_manager.h"
#include "log_manager.h"

#include "control_sim.h"

// Timing parameters
static uint32_t g_currExecTime = 0;

// Test parameters
uint32_t g_ui32SecCounter;
uint32_t g_ui32Counter2;

bool g_bBootSequenceStart = false;
uint32_t g_ui32SeqStart = 0;
bool g_bBootFinished = false;
uint32_t g_ui32BootFinishedStart = 0;
bool g_bOnHomeForm = false;
uint32_t g_ui32HomeStarted = 0;

uint32_t ui32GraphCount = 0;

float val, t2val, t3val, vinVal, voutVal;

float g_f32SawtoothVal = 0;
float g_f32SineVal = 0; 

bool g_bLogStarted = false;
uint32_t g_ui32LogStart = 0;

int32_t g_i32LogId = 0;

char statusStr[12] = "NOMINAL";
char dateStr[11] = "27/04/2026";
char timeStr[9] = "03:34:34";

static void onBootSequenceStart(uint32_t arg) {
	g_bBootSequenceStart = true;
	g_ui32SeqStart = GetExecTimeMs();
}

static void onBootFinished(uint32_t arg) {
	g_bBootFinished = true;
	g_bBootSequenceStart = false;
	g_ui32BootFinishedStart = GetExecTimeMs();
}

static void onHomeRendered(EventParam_t arg) {
	g_ui32HomeStarted = GetExecTimeMs();
	g_bOnHomeForm = true;

	RTC_getFormattedDate(dateStr, sizeof(dateStr));
	Event_Post(EVT_SYS_DATE_CHANGED, (EventParam_t){.str = dateStr});
}

static void onLogStart(EventParam_t param) {
	char *HEADER[] = {"Date", "Time", "Time elapsed [ms]", "Sine", "Sawtooth"};

	g_i32LogId = LogManager_StartLog(DRIVE_USB_ID, "this is just a long name", HEADER, 4);
	if(g_i32LogId == -1) return;

	g_bLogStarted = true;
	g_ui32LogStart = GetExecTimeMs();
}

static void onLogStop(EventParam_t param) {
	g_bLogStarted = false;
}

void controlSimulatorInit(void)
{
	// Initialize variables
	g_ui32SecCounter = 2;
	g_ui32Counter2 = 0;
	srand(GetExecTimeMs());
	Event_Subscribe(EVT_CMD_START_BOOT_SEQ, ( EventHandler_fn ) onBootSequenceStart);
	Event_Subscribe(EVT_SYS_BOOT_FINISHED, (EventHandler_fn) onBootFinished);
	Event_Subscribe(EVT_SYS_USB_START_LOG, (EventHandler_fn) onLogStart);
	Event_Subscribe(EVT_SYS_USB_STOP_LOG, (EventHandler_fn) onLogStop);

	Event_Subscribe(EVT_SYS_SHOW_HOME_FORM, (EventHandler_fn)onHomeRendered);
}

void controlSimulatiorTask(void) {

    g_currExecTime = GetExecTimeMs();

	// Update all clocks
	if(g_currExecTime % 1000 == 0) {
		RTC_getFormattedTime(timeStr, sizeof(timeStr));
		Event_Post(EVT_SYS_TIME_CHANGED, (EventParam_t){.str = timeStr});
	}

	if(g_currExecTime % 200 == 0)
	{
		ui32GraphCount++;
		//uint32_t val = rand() % 70;
		// float val =  25.0f * sin((double)g_currExecTime / (700.0)) + 50.0;
		g_f32SineVal =  25.0f * sin((double)ui32GraphCount / 100) + 50.0;
		
		Event_Post(EVT_SYS_NEW_GRAPH_VALUE, (EventParam_t){.f32 = g_f32SineVal});
	}

	if(g_currExecTime % 200 == 0)
    {
        // 1. Define the period (how many ticks for one full wave cycle)
        // 4400 roughly matches the frequency of your previous sine wave
        // const uint32_t PERIOD = 4400.0f / 2.0f; 
        const uint32_t PERIOD = 200.0f; 
        
        // 2. Define the Peak-to-Peak amplitude (75 max - 25 min = 50)
        const float AMPLITUDE = 50.0f;
        
        // 3. Define the DC Offset (the minimum value the wave hits)
        const float OFFSET = 25.0f;

        // Calculate the sawtooth using pure integer math.
        // We multiply first, then divide, to prevent integer truncation to 0.
        // float val = (((g_currExecTime % PERIOD) * AMPLITUDE) / PERIOD) + OFFSET;
        g_f32SawtoothVal = (((ui32GraphCount % PERIOD) * AMPLITUDE) / PERIOD) + OFFSET;
        
        bool ret = Event_Post(EVT_SYS_NEW_SAWTOOTH_VALUE, (EventParam_t){.f32 = g_f32SawtoothVal});
    }

	if(g_bOnHomeForm) {
	// if(false) {
		if(g_currExecTime % 500 == 0) {
			val = ((float)rand() / RAND_MAX) * 2.0 - 1.0;
			val = 25.0f + val;
			Event_Post(EVT_SYS_CURRENT_VAL_CHANGED, (EventParam_t){.f32 = val});
		}
		if(g_currExecTime % 700 == 0) {
			t2val = ((float)rand() / RAND_MAX) * 4.0 - 2.0;
			t2val = 44.0f + t2val;
			Event_Post(EVT_SYS_T1_VAL_CHANGED, (EventParam_t){.f32 = t2val});
		}
		if(g_currExecTime % 1000 == 0) {
			t3val = ((float)rand() / RAND_MAX) * 3.0 - 1.0;
			t3val = 33.0f + t3val;
			Event_Post(EVT_SYS_VOLTAGE_VAL_CHANGED, (EventParam_t){.f32 = t3val});
		}
	}

	if(g_bLogStarted) {
		if(g_currExecTime % 500 == 0) {
			char val[50];
			char dateStr[12];
			char timeStr[11];

			RTC_getFormattedTime(timeStr, sizeof(timeStr));
			RTC_getFormattedDate(dateStr, sizeof(dateStr));

			snprintf(val, sizeof(val), "%s,%s,%u,%.2f,%.2f\n", dateStr, timeStr, g_currExecTime, g_f32SineVal, g_f32SawtoothVal);

			LogManager_AddLineToLog(g_i32LogId, val);

			Event_Post(EVT_SYS_USB_LOG_ADDED, (EventParam_t){.ptr = NULL});
		}
	}

}


