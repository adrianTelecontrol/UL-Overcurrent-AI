#include "data_model.h"
#include "event_engine.h"
#include <string.h>

// Definición de la variable global (inicializada en 0)
LogDataRow_t g_LatestReadings = {0};

// =========================================================================
// CALLBACKS DE ACTUALIZACIÓN (Suscriptos a Eventos CAN)
// =========================================================================

// --- VOLTAJES ---
static void onUpdate_VoltagePrimary(EventParam_t arg) {
    g_LatestReadings.voltagePrimary = arg.f32;
}

static void onUpdate_VoltageSecondary(EventParam_t arg) {
    g_LatestReadings.voltageSecondary = arg.f32;
}

// --- CORRIENTES ---
static void onUpdate_CurrentPrimary(EventParam_t arg) {
    g_LatestReadings.currentPrimary = arg.f32;
}

static void onUpdate_CurrentSecondary(EventParam_t arg) {
    // Nota: usando SECUNDARY con 'U' basado en tu archivo inst_can_buffer
    g_LatestReadings.currentSecondary = arg.f32;
}

// --- TEMPERATURAS DE SONDAS ---
static void onUpdate_TempProbeMain(EventParam_t arg) {
    g_LatestReadings.tempProbeMain = arg.f32;
}

static void onUpdate_TempProbeSecondary(EventParam_t arg) {
    g_LatestReadings.tempProbeSec = arg.f32;
}

// --- TEMPERATURAS DE TRANSFORMADOR ---
static void onUpdate_TempTxPrimary(EventParam_t arg) {
    g_LatestReadings.tempTxPrimary = arg.f32;
}

static void onUpdate_TempTxSecondary(EventParam_t arg) {
    g_LatestReadings.tempTxSecondary = arg.f32;
}

// --- TEMPERATURAS DE CABLES Y CJC ---
static void onUpdate_TempCableA(EventParam_t arg) {
    g_LatestReadings.tempCableA = arg.f32;
}

static void onUpdate_TempCableB(EventParam_t arg) {
    g_LatestReadings.tempCableB = arg.f32;
}

static void onUpdate_TempCJC(EventParam_t arg) {
    g_LatestReadings.tempCJC = arg.f32;
}

// =========================================================================
// FUNCIÓN DE INICIALIZACIÓN
// =========================================================================

void DataModel_Init(void) {
    // Aseguramos que la estructura inicie limpia
    memset(&g_LatestReadings, 0, sizeof(LogDataRow_t));

    // Suscripción de Voltajess
    Event_Subscribe(EVT_CAN_INST_VOLTAGE_PRIMARY,   (EventHandler_fn)onUpdate_VoltagePrimary);
    Event_Subscribe(EVT_CAN_INST_VOLTAGE_SECONDARY, (EventHandler_fn)onUpdate_VoltageSecondary);

    // Suscripción de Corrientes
    Event_Subscribe(EVT_CAN_INST_CURRENT_PRIMARY,   (EventHandler_fn)onUpdate_CurrentPrimary);
    Event_Subscribe(EVT_CAN_INST_CURRENT_SECUNDARY, (EventHandler_fn)onUpdate_CurrentSecondary);

    // Suscripción de Temperaturas
    Event_Subscribe(EVT_CAN_INST_TEMP_PROBE_MAIN,      (EventHandler_fn)onUpdate_TempProbeMain);
    Event_Subscribe(EVT_CAN_INST_TEMP_PROBE_SECONDARY, (EventHandler_fn)onUpdate_TempProbeSecondary);
    
    Event_Subscribe(EVT_CAN_INST_TEMP_TX_PRIMARY,      (EventHandler_fn)onUpdate_TempTxPrimary);
    Event_Subscribe(EVT_CAN_INST_TEMP_TX_SECONDARY,    (EventHandler_fn)onUpdate_TempTxSecondary);
    
    Event_Subscribe(EVT_CAN_INST_TEMP_CABLE_A,         (EventHandler_fn)onUpdate_TempCableA);
    Event_Subscribe(EVT_CAN_INST_TEMP_CABLE_B,         (EventHandler_fn)onUpdate_TempCableB);
    Event_Subscribe(EVT_CAN_INST_TEMP_CJC,             (EventHandler_fn)onUpdate_TempCJC);
}