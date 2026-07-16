#ifndef GFX_CANVAS_H
#define GFX_CANVAS_H

#include <stdint.h>
#include <stdbool.h>

#include "gui_core.h"

gfx_GenericWidgetNode* canvasCreateNode(gfx_GenericWidget *wd);

bool canvasInsertAtBottom(gfx_GenericWidgetNode** head, gfx_GenericWidget *wd);

bool canvasInsertAtTop(gfx_GenericWidgetNode** head, gfx_GenericWidget *wd);

bool canvasInsertAtPosition(gfx_GenericWidgetNode** head, gfx_GenericWidget *wd, uint8_t pos);

bool canvasDeleteAtBeginning(gfx_GenericWidgetNode** head);

bool canvasDeleteAtEnd(gfx_GenericWidgetNode** head);

bool canvasDeleteAtPosition(gfx_GenericWidgetNode** head, uint8_t pos);

bool canvasReverseTraverseList(gfx_GenericWidgetNode** head);

bool canvasForwardTraverseList(gfx_GenericWidgetNode** head);

uint32_t canvasGetListSizeBytes(gfx_GenericWidgetNode** head);

void canvasPrintListSize(gfx_GenericWidgetNode** head);

#endif // GFX_CANVAS_H 



