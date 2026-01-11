//   Copyright 2024 Aidan Holmes
//   List Gadget Wrapper

#ifndef __GAD_LISTVIEW_HDR
#define __GAD_LISTVIEW_HDR

#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/exec.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include "app.h"

typedef struct ListViewNode {
	struct Node _n;
	void *listItemData;
} ListViewNode;

typedef struct ListViewGadget {
    struct List lvList ;
} ListViewGadget;

// This function MUST be called before calling any of the functions below.
// Can be called before or after openAppWindow or addAppGadget.
// Calling this function will allocate additional data and setup the list view for subsequent calls to add or remove list items
// Returns 0 on success
int lvInitialise(AppGadget *lvg);

// Adds a new item to the list view. Pos can be -1 to add to the end of the list. Pos is zero based index which can be used to specify a position
// to add the new node. Using 0 will always add to the top (head) of the list view.
struct Node *lvAddEntry(AppGadget *lvg, int pos, char *szEntry, void *pData);

// This function will highlight a list view item. Pos is the list index to highlight
void lvHighlightItem(Wnd *myWnd, AppGadget *lvg, ULONG pos);

// This function will update the gadget with any new changes made to the list view.
void lvUpdateList(Wnd *myWnd, AppGadget *lvg);

// Deletes all items in the list and deallocates memory
// Calls lvResetList on the List struct to allow new entries
void lvFreeList(AppGadget *lvg);

// Fetches the name of the list item selected at code position
// Returns NULL if not found
char *lvGetListText(AppGadget *lvg, ULONG code);

// Get the data for list item at code position
// returns NULL if not found (note this may be valid if the data wasn't set)
void *lvGetListData(AppGadget *lvg, ULONG code) ;

// Returns the number of items in the list view
ULONG lvGetItemCount(AppGadget *lvg);

// Utility call to initialise the List struct. This is the same as NewNode in later V50
// This will NOT free and delete the existing list. Call lvFreeList first if removing the list and resetting
void lvResetList(struct List *l);

// Remove a specific entry in the list view at pos
void lvDeleteEntry(AppGadget *lvg, int pos);

#endif