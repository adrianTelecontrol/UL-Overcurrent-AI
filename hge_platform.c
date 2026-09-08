#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "event_engine.h"
#include "file_manager.h"
#include "forms_manager.h"
#include "hge_platform.h"
#include "tiva_log.h"

static HgePlatformCallback_t g_pFullRepaintCallback;
static HgePlatformCallback_t g_pThemeChangeCallback;
static bool g_bFullRepaintSubscribed;
static bool g_bThemeChangeSubscribed;

static void HgePlatform_OnFullRepaint(EventParam_t ignored) {
  (void)ignored;

  if (g_pFullRepaintCallback != NULL) {
    g_pFullRepaintCallback();
  }
}

static void HgePlatform_OnThemeChange(EventParam_t ignored) {
  (void)ignored;

  if (g_pThemeChangeCallback != NULL) {
    g_pThemeChangeCallback();
  }
}

void HgePlatform_RequestFullRepaint(void) {
  if (!Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL})) {
    TIVA_LOGE("HGE_PLATFORM", "Full repaint event queue full");
  }
}

void HgePlatform_SubscribeFullRepaint(HgePlatformCallback_t callback) {
  g_pFullRepaintCallback = callback;

  if (!g_bFullRepaintSubscribed) {
    g_bFullRepaintSubscribed =
        Event_Subscribe(EVT_CMD_FULL_REPAINT, HgePlatform_OnFullRepaint);
    if (!g_bFullRepaintSubscribed) {
      TIVA_LOGE("HGE_PLATFORM", "Full repaint subscription failed");
    }
  }
}

void HgePlatform_SubscribeThemeChange(HgePlatformCallback_t callback) {
  g_pThemeChangeCallback = callback;

  if (!g_bThemeChangeSubscribed) {
    g_bThemeChangeSubscribed =
        Event_Subscribe(EVT_CMD_CHANGE_THEME, HgePlatform_OnThemeChange);
    if (!g_bThemeChangeSubscribed) {
      TIVA_LOGE("HGE_PLATFORM", "Theme subscription failed");
    }
  }
}

void HgePlatform_CompositeFrame(void *framebuffer) {
  FormManager_CompositeFrame((pixel16_t *)framebuffer);
}

bool HgePlatform_CheckSoftwareDirty(void) {
  return FormManager_CheckSoftwareDirty();
}

bool HgePlatform_CheckHardwareDirty(void) {
  return FormManager_CheckHardwareDirty();
}

void HgePlatform_RenderEveComponents(void) {
  FormManager_RenderEVEComponents();
}

bool HgePlatform_DeliverGesture(TouchStatus touch, gesture_type_e gesture) {
  return FormManager_HandleGesture(touch, gesture);
}

bool HgePlatform_LoadDefaultBdfFont(const char *path, void *font,
                                    uint16_t firstChar, uint16_t lastChar) {
  return FM_FetchBDF(DRIVE_SD_ID, path, (BDF_Font_t *)font, firstChar,
                     lastChar);
}

uint8_t HgePlatform_DefaultStorageDrive(void) {
  return DRIVE_SD_ID;
}

const char *HgePlatform_DrivePath(uint8_t drive) {
  return FM_getDriveString(drive);
}

const char *HgePlatform_FileResultString(int32_t result) {
  return FM_StringFromFResult((FRESULT)result);
}
