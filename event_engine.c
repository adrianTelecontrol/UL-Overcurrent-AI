
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "helpers.h"
#include "event_engine.h"

#define EVENT_QUEUE_SIZE	64
#define MAX_SUBSCRIBERS_PER_EVENT	10

static SystemEvent_t eventQueue[EVENT_QUEUE_SIZE];
static volatile uint32_t head = 0;
static volatile uint32_t tail = 0;

static const char *TAG = "eventEngine";

typedef struct {
	EventHandler_fn handlers[MAX_SUBSCRIBERS_PER_EVENT];
	uint8_t count;
} Subscription_t;

static Subscription_t registry[NUM_EVENTS];

bool Event_Init(void)
{
	head = 0;
	tail = 0;
	int i = 0;
	for(; i < NUM_EVENTS; i++) {
		registry[i].count = 0;
	}

	return true;
}

bool Event_Post(EventID_e id, EventParam_t arg) {
    // FIX: Subtract 1 to create a valid bitmask 
    // (NOTE: EVENT_QUEUE_SIZE MUST be a power of 2 for this to work!)
    uint16_t nextHead = (head + 1) & (EVENT_QUEUE_SIZE - 1);

    if(nextHead == tail) {
		TIVA_LOGE(TAG, "Event buffer overflow");
        return false;   // La cola esta llena
    }

    // CRITICAL for bare-metal: Disable interrupts here if this 
    // function can ever be called from inside an ISR (like a UART RX)!
    
    eventQueue[head].id = id;
    eventQueue[head].arg = arg;
    head = nextHead;
    
    // Re-enable interrupts here if you disabled them
    
    return true;
}

bool Event_Receive(SystemEvent_t *pEvent) {
	if(head == tail){
		return false;
	}
	
	*pEvent = eventQueue[tail];
	tail = (tail + 1) % EVENT_QUEUE_SIZE;

	return true;
}

// Module registration 
bool Event_Subscribe(EventID_e id, EventHandler_fn handler)
{
	if(id >= NUM_EVENTS) return false;

	uint8_t currentCount = registry[id].count;
	if(currentCount < MAX_SUBSCRIBERS_PER_EVENT) {
		registry[id].handlers[currentCount] = handler;
		registry[id].count++;
		return true;
	}
	return false;
}

bool Event_Dispatch(void)
{
    SystemEvent_t evt;

    while(Event_Receive(&evt)) {
        uint8_t subCount = registry[evt.id].count;

        uint8_t i = 0;
        for(; i < subCount; i++)
        {
            if(registry[evt.id].handlers[i] != NULL) 
            {
                // Pasamos la unión completa al callback
                registry[evt.id].handlers[i](evt.arg);
            }
        }
    }

	return true;
}

