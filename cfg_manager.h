#ifndef CFG_MANAGER_H
#define CFG_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

// =========================================================================
// PROTOTIPOS PÚBLICOS
// =========================================================================

bool CfgManager_SendConfigFileToInst(const char *filePath);

void CfgManager_RequestConfigFromInst(void);

#endif // CFG_MANAGER_H