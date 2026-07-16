#include <stdlib.h>
#include <string.h>

#include "gui_canvas.h"


gfx_GenericWidgetNode* canvasCreateNode(gfx_GenericWidget *wd)
{
    gfx_GenericWidgetNode* node = (gfx_GenericWidgetNode *)malloc(sizeof(gfx_GenericWidgetNode));
    if(node == NULL)
        return NULL;

    node->psNext = NULL;
    node->psPrev = NULL;

    // Store the pointer directly — do NOT copy the widget data.
    // The canvas is a view over the widgets, not an owner of them.
    node->sWidget.eWidgetType = wd->eWidgetType;
    node->sWidget.pvWidget    = wd->pvWidget;

    return node;
}

bool canvasInsertAtBottom(gfx_GenericWidgetNode** head, gfx_GenericWidget *wd)
{
    gfx_GenericWidgetNode *newNode = canvasCreateNode(wd);

    // Check if head is empty
    if(*head == NULL)
    {
        *head = newNode;
    }

    newNode->psNext = *head;
    (*head)->psPrev = newNode;
    *head = newNode;

    return true;
}

bool canvasInsertAtTop(gfx_GenericWidgetNode** head, gfx_GenericWidget *wd)
{
    canvasPrintListSize(head);
    gfx_GenericWidgetNode *newNode = canvasCreateNode(wd);

    // Check if head is empty
    if(*head == NULL)
    {
        *head = newNode;
        return true;
    }

    gfx_GenericWidgetNode *temp = *head;
    while(temp->psNext != NULL)
    {
        temp = temp->psNext;
    }
    temp->psNext = newNode;
    newNode->psPrev = temp;

    return true;
}

bool canvasInsertAtPosition(gfx_GenericWidgetNode** head, gfx_GenericWidget *wd, uint8_t pos)
{
    if(pos == 1)
    {
        canvasInsertAtBottom(head, wd);
        return true;
    }
    gfx_GenericWidgetNode *newNode = canvasCreateNode(wd);
    gfx_GenericWidgetNode *temp = *head;

    uint8_t i = 0;
    for(i = 0; temp != NULL && i < pos - 1; i++)
    {
        temp = temp->psNext;
    }

    if(temp == NULL)
    {
        // System_printf("Position greater than the number of nodes");
        // System_flush();
        return false;
    }

    newNode->psNext = temp->psNext;
    newNode->psPrev = temp;
    if(temp->psNext != NULL)
    {
        temp->psNext->psPrev = newNode;
    }
    temp->psNext = newNode;

    return true;
}

bool canvasDeleteAtBeginning(gfx_GenericWidgetNode** head)
{
    if(*head == NULL)
    {
        return true;
    }

    gfx_GenericWidgetNode *temp = *head;
    *head = (*head)->psNext;
    if(*head != NULL)
    {
        (*head)->psPrev = NULL;
    }
    if(temp->sWidget.pvWidget != NULL)
        free(temp->sWidget.pvWidget);
    free(temp);

	return true;
}

bool canvasDeleteAtEnd(gfx_GenericWidgetNode** head)
{
    if(*head == NULL)
    {
        return true;
    }

    gfx_GenericWidgetNode *temp = *head;
    if(temp->psNext == NULL)
    {
        *head = NULL;
        free(temp);
        return true;
    }
    while (temp->psNext != NULL) 
    {
        temp = temp->psNext;
    }
    temp->psPrev->psNext = NULL;
    if(temp->sWidget.pvWidget != NULL)
        free(temp->sWidget.pvWidget);
    free(temp);

    return true;
}

bool canvasDeleteAtPosition(gfx_GenericWidgetNode** head, uint8_t pos)
{
    if(*head == NULL)
    {
        return true;
    }
    gfx_GenericWidgetNode* temp = *head;
    if(pos == 1)
    {
        canvasDeleteAtBeginning(head);
        return true;
    }
    unsigned int i = 0;
    for (i = 0; temp != NULL && i < pos - 1; i++) 
    {
        temp = temp->psNext;
    }
    if(temp == NULL)
    {
        // System_printf("Position greater than the number of nodes");
        // System_flush();
        return false;
    }
    if(temp->psNext != NULL)
    {
        temp->psNext->psPrev = temp->psPrev;
    }
    if(temp->psPrev != NULL)
    {
        temp->psPrev->psNext = temp->psNext;
    }

    if(temp->sWidget.pvWidget != NULL)
        free(temp->sWidget.pvWidget);
    free(temp);
    return true;
}

bool canvasReverseTraverseList(gfx_GenericWidgetNode** head)
{
    if(*head == NULL)
    {
        // System_printf("List is empty\n");
        // System_flush();
        return false;
    }
    
    gfx_GenericWidgetNode *temp = *head;
    
    // Move to the last node
    while(temp->psNext != NULL)
    {
        temp = temp->psNext;
    }
    
    uint8_t position = 0;
    
    // Traverse backwards using psPrev
    while(temp != NULL)
    {
        temp = temp->psPrev;
        position++;
    }
    
    return true;
}

bool canvasForwardTraverseList(gfx_GenericWidgetNode** head)
{
    if(*head == NULL)
    {
        // System_printf("List is empty\n");
        // System_flush();
        return false;
    }
    
    gfx_GenericWidgetNode *temp = *head;
    uint8_t position = 0;
    
    while(temp != NULL)
    {
        temp = temp->psNext;
        position++;
    }
    
    return true;
}

uint32_t canvasGetListSizeBytes(gfx_GenericWidgetNode** head)
{
    if(*head == NULL)
        return 0;
    
    uint32_t totalSize = 0;
    gfx_GenericWidgetNode *temp = *head;
    
    while(temp != NULL)
    {
        // Size of the node itself
        totalSize += sizeof(gfx_GenericWidgetNode);
        
        // Size of the widget data
        if(temp->sWidget.pvWidget != NULL)
        {
            switch(temp->sWidget.eWidgetType)
            {
                case WD_TYPE_BUTTON:
                    totalSize += sizeof(gfx_Button);
                    break;
                    
                case WD_TYPE_RECT:
                    totalSize += sizeof(gfx_Rectangle);
                    break;
                    
                default:
                    break;
            }
        }
        
        temp = temp->psNext;
    }
    
    return totalSize;
}

void canvasPrintListSize(gfx_GenericWidgetNode** head)
{
    if(*head == NULL)
    {
        // System_printf("List is empty (0 bytes)\n");
        // System_flush();
        return;
    }
    
    uint32_t totalSize = 0;
    uint32_t nodeCount = 0;
    uint32_t buttonCount = 0;
    uint32_t rectCount = 0;
    
    gfx_GenericWidgetNode *temp = *head;
    
    while(temp != NULL)
    {
        nodeCount++;
        totalSize += sizeof(gfx_GenericWidgetNode);
        
        if(temp->sWidget.pvWidget != NULL)
        {
            switch(temp->sWidget.eWidgetType)
            {
                case WD_TYPE_BUTTON:
                    totalSize += sizeof(gfx_Button);
                    buttonCount++;
                    break;
                    
                case WD_TYPE_RECT:
                    totalSize += sizeof(gfx_Rectangle);
                    rectCount++;
                    break;
                    
                default:
                    break;
            }
        }
        
        temp = temp->psNext;
    }
    
}




