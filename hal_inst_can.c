#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
// Driverlib Includes
#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/can.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"

#include "helpers.h"
#include "inst_can_buffer.h"

#include "hal_inst_can.h"

static const char TAG[] = "HAL_CAN";

// CAN0 on TM4C1294NCPDT uses PA0 (RX) and PA1 (TX).
#define RX_OBJECT_ID    1
#define TX_OBJECT_ID    2

static HAL_CAN_RxCallback_fn g_pfnRxCallback = NULL;

// Tracks whether the TX mailbox is occupied.
// Set true in HAL_CAN_Transmit, cleared by the TX ISR.
static volatile bool g_bTxBusy = false;

static void onCANMessageReceived(HAL_CAN_Msg_t *msg) {
    /*HAL_CAN_Msg_t reply;

    reply.id         = 0x200;
    reply.length     = msg->length;
    reply.isExtended = false;

	uint8_t i = 0;
    for (; i < msg->length; i++) {
        reply.data[i] = msg->data[i];
    }

    HAL_CAN_Transmit(&reply);*/

	InstCanBuffer_Push(msg);
}

bool HAL_CAN_Init(uint32_t systemClock, uint32_t bitrate) {
	// Initialize Instrumentation CAN buffer
	InstCanBuffer_Init();
	// 1. Enable and wait for GPIOA and CAN0 clocks.
    //    (GPIOB is NOT needed for CAN0 on this device — removed.)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) {}
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_CAN0))  {}

    // 2. Mux PA0/PA1 to CAN0.
    GPIOPinConfigure(GPIO_PA0_CAN0RX);
    GPIOPinConfigure(GPIO_PA1_CAN0TX);
    GPIOPinTypeCAN(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // 3. Initialise the CAN controller (resets all message objects).
    CANInit(CAN0_BASE);

    // 4. Set bitrate.
    uint32_t ui32ActualRate = 0; 
    ui32ActualRate = CANBitRateSet(CAN0_BASE, systemClock, bitrate);
	TIVA_LOGI(TAG, "Value: %u", ui32ActualRate);
	   
    // 5. Configure the RX message object BEFORE enabling the bus.
    //    ID = 0, Mask = 0, no MSG_OBJ_USE_ID_FILTER  →  receive ALL traffic.
    //    (MSG_OBJ_USE_ID_FILTER with mask 0 only passes ID 0, which is wrong.)
// Configure the RX message object
    tCANMsgObject sRxMsg;
    sRxMsg.ui32MsgID     = 0;
    sRxMsg.ui32MsgIDMask = 0; // A mask of 0 means "match everything"
    
    // You MUST include MSG_OBJ_USE_ID_FILTER for the mask of 0 to be applied!
    sRxMsg.ui32Flags     = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER; 
    
    sRxMsg.ui32MsgLen    = 8;
    sRxMsg.pui8MsgData   = NULL;
    CANMessageSet(CAN0_BASE, RX_OBJECT_ID, &sRxMsg, MSG_OBJ_TYPE_RX);

    // 6. Put the controller on the bus.
    CANEnable(CAN0_BASE);

    // 7. Enable CAN interrupts (master + error + status).
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR);
    IntEnable(INT_CAN0);
    // NOTE: IntMasterEnable() must be called in main() if not already done.

	HAL_CAN_SetRxCallback(onCANMessageReceived);

    return true;
}

void HAL_CAN_SetRxCallback(HAL_CAN_RxCallback_fn callback) {
    g_pfnRxCallback = callback;
}

bool HAL_CAN_IsTxBusy(void) {
    return g_bTxBusy;
}

bool HAL_CAN_Transmit(HAL_CAN_Msg_t *msg) {
    if (msg == NULL || msg->length > 8) return false;

    // Refuse to overwrite a pending transmission.
    if (g_bTxBusy) return false;

    tCANMsgObject sTxMsg;
    sTxMsg.ui32MsgID     = msg->id;
    sTxMsg.ui32MsgIDMask = 0;                       /* not used on TX */
    sTxMsg.ui32Flags     = MSG_OBJ_TX_INT_ENABLE;

    if (msg->isExtended) {
        sTxMsg.ui32Flags |= MSG_OBJ_EXTENDED_ID;
    }

    sTxMsg.ui32MsgLen  = msg->length;
    sTxMsg.pui8MsgData = msg->data;

    g_bTxBusy = true;
    CANMessageSet(CAN0_BASE, TX_OBJECT_ID, &sTxMsg, MSG_OBJ_TYPE_TX);
	//g_bTxBusy = false;
    return true;
}

/*
void CAN0IntHandler(void) {
    uint32_t ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);
    tCANMsgObject sCANMessage;

    if (ui32Status == CAN_INT_INTID_STATUS) {
        uint32_t ui32CtrlStatus = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);

        if (ui32CtrlStatus & 0x10) {
            // RXOK — a frame landed in a mailbox, go read it
            uint8_t pui8MsgData[8];
            sCANMessage.pui8MsgData = pui8MsgData;

            CANMessageGet(CAN0_BASE, RX_OBJECT_ID, &sCANMessage, 1);

            if (g_pfnRxCallback != NULL) {
                HAL_CAN_Msg_t rxMsg;
                rxMsg.id         = sCANMessage.ui32MsgID;
                rxMsg.length     = sCANMessage.ui32MsgLen;
                rxMsg.isExtended = (sCANMessage.ui32Flags & MSG_OBJ_EXTENDED_ID) != 0;

				uint8_t i = 0;
                for (; i < rxMsg.length; i++) {
                    rxMsg.data[i] = pui8MsgData[i];
                }
                g_pfnRxCallback(&rxMsg);
            }
        }

        if (ui32CtrlStatus & 0x08) {
            // TXOK — a frame was transmitted successfully
            g_bTxBusy = false;
        }

    } else if (ui32Status == RX_OBJECT_ID) {
        // Mailbox interrupt path — same read logic as above
        uint8_t pui8MsgData[8];
        tCANMsgObject sCANMessage;
        sCANMessage.pui8MsgData = pui8MsgData;

        CANMessageGet(CAN0_BASE, RX_OBJECT_ID, &sCANMessage, 1);


        if (g_pfnRxCallback != NULL) {
            HAL_CAN_Msg_t rxMsg;
            rxMsg.id         = sCANMessage.ui32MsgID;
            rxMsg.length     = sCANMessage.ui32MsgLen;
            rxMsg.isExtended = (sCANMessage.ui32Flags & MSG_OBJ_EXTENDED_ID) != 0;

			uint8_t i = 0;
            for (; i < rxMsg.length; i++) {
                rxMsg.data[i] = pui8MsgData[i];
            }
            g_pfnRxCallback(&rxMsg);
        }

    } else if (ui32Status == TX_OBJECT_ID) {
        CANIntClear(CAN0_BASE, TX_OBJECT_ID);
        g_bTxBusy = false;
    }
} */


void CAN0IntHandler(void) {
    uint32_t ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    if (ui32Status == CAN_INT_INTID_STATUS) {
        // Status interrupt — just clear it, do NOT try to read the mailbox here.
        // The mailbox interrupt (ui32Status == RX_OBJECT_ID) will follow
        // shortly with the actual frame data.
        uint32_t ui32CtrlStatus = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
        (void)ui32CtrlStatus;  // reading it clears the interrupt

    } else if (ui32Status == RX_OBJECT_ID) {
        // Mailbox interrupt — frame is now fully in message obj 	ect RAM.
        uint8_t pui8MsgData[8];
        tCANMsgObject sCANMessage;
        sCANMessage.pui8MsgData = pui8MsgData;

        CANMessageGet(CAN0_BASE, RX_OBJECT_ID, &sCANMessage, 1);

        if (g_pfnRxCallback != NULL) {
            HAL_CAN_Msg_t rxMsg;
            rxMsg.id         = sCANMessage.ui32MsgID;
            rxMsg.length     = sCANMessage.ui32MsgLen;
            rxMsg.isExtended = (sCANMessage.ui32Flags & MSG_OBJ_EXTENDED_ID) != 0;

			uint8_t i = 0;
            for (; i < rxMsg.length; i++) {
                rxMsg.data[i] = pui8MsgData[i];
            }
            g_pfnRxCallback(&rxMsg);
        }

    } else if (ui32Status == TX_OBJECT_ID) {
        CANIntClear(CAN0_BASE, TX_OBJECT_ID);
        g_bTxBusy = false;
    } else {
        g_bTxBusy = false;
	}
} 


