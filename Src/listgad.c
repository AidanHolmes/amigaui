//   Copyright 2024 Aidan Holmes
//   List Gadget Wrapper

#include "listgad.h"
#include <stdio.h>

void lvResetList(struct List *l)
{
    l->lh_Tail = NULL;
    l->lh_Head = (struct Node *)&l->lh_Tail;
    l->lh_TailPred = (struct Node *)&l->lh_Head;
}

int lvInitialise(AppGadget *lvg)
{
    ListViewGadget *lvgData = NULL;
    
    if (!lvg) return 5;
  
    lvg->data = NULL;
      
    lvgData = AllocMem(sizeof(ListViewGadget),0);
    if (!lvgData) return 5;
    
    // The list setup dance
    lvResetList(&lvgData->lvList);
    
    lvg->data = lvgData;
    
    return 0;
}

void lvDeleteEntry(AppGadget *lvg, int pos)
{
    ListViewGadget *lvgData = NULL ;
    int i=0;
    struct Node *n = NULL ;
    
    lvgData = (ListViewGadget *)lvg->data ;
    
    if (pos == -1){
        n = RemTail(&lvgData->lvList) ;
    }else if(pos == 0){
        n = RemHead(&lvgData->lvList) ;
    }else{
        i=0;
        for(n=lvgData->lvList.lh_Head; n->ln_Succ != NULL; n = n->ln_Succ){
            if (i == pos){
                Remove(n);
                break;
            }
            i++;
        }    
    }
    if (n){
        FreeMem(n, sizeof(ListViewNode));
    }
}

struct Node *lvAddEntry(AppGadget *lvg, int pos, char *szEntry, void *pData)
{
    ListViewGadget *lvgData = NULL ;
    struct ListViewNode *newLvEntry = NULL;
    struct Node *newEntry, *n = NULL ;
    int i=0, inserted=0;
    
    if (!lvg) return NULL;
    if (!(lvg->data)) return NULL;
    
    lvgData = (ListViewGadget *)lvg->data ;
    
    newLvEntry = AllocMem(sizeof(ListViewNode),0);
    if (!newLvEntry) return NULL;
	newEntry = (struct Node *)newLvEntry;
    
    newEntry->ln_Succ = NULL;
    newEntry->ln_Pred = NULL;
    newEntry->ln_Type = 100;
    newEntry->ln_Pri = 0;
    newEntry->ln_Name = szEntry; // Maybe allocate and strcpy?
	newLvEntry->listItemData = pData ;
    
    if (pos == -1){
        AddTail(&lvgData->lvList, newEntry) ;
    }else if(pos == 0){
        AddHead(&lvgData->lvList, newEntry) ;
    }else{
        i=0;
        for(n=lvgData->lvList.lh_Head; n->ln_Succ != NULL; n = n->ln_Succ){
            if (i == pos){
                Insert(&lvgData->lvList, newEntry, n->ln_Pred);
                inserted=1;
                break;
            }
            i++;
        }
        if (inserted == 0){
            AddTail(&lvgData->lvList, newEntry) ;
        }
    }
    /*
    printf("List %p contains: ", &lvgData->lvList);
    for(n=lvgData->lvList.lh_Head; n->ln_Succ != NULL; n = n->ln_Succ){
        printf(" (n:%p -> %s, pred %p, succ %p)", n, n->ln_Name, n->ln_Pred, n->ln_Succ) ;
    }
    printf("\n");
    */
    return newEntry;
}

void lvHighlightItem(Wnd *myWnd, AppGadget *lvg, ULONG pos)
{
	struct Library *GadToolsBase = NULL ;
	ListViewGadget *lvgData = NULL;

	GadToolsBase = myWnd->app->gadtools;
	
    if (lvg){
        if (lvg->data){
            lvgData = (ListViewGadget *)lvg->data ;
            
            GT_SetGadgetAttrs(lvg->gadget, myWnd->appWindow, NULL, GTLV_Selected, pos, TAG_END);
        }
    }
}

void lvUpdateList(Wnd *myWnd, AppGadget *lvg)
{
	struct Library *GadToolsBase = NULL ;
    ListViewGadget *lvgData = NULL;

	GadToolsBase = myWnd->app->gadtools;

    if (lvg){
        if (lvg->data){
            lvgData = (ListViewGadget *)lvg->data ;
            
            GT_SetGadgetAttrs(lvg->gadget, myWnd->appWindow, NULL, GTLV_Labels, ~0, TAG_END);
            
            GT_SetGadgetAttrs(lvg->gadget, myWnd->appWindow, NULL, GTLV_Labels, &lvgData->lvList, TAG_END);
        }
    }
}

void lvFreeList(AppGadget *lvg)
{
    struct Node *workNode = NULL;
    struct Node *nextNode = NULL; 
    ListViewGadget *lvgData = NULL;
    
    if (lvg){
        if (lvg->data){
            lvgData = (ListViewGadget *)lvg->data ;
            workNode = lvgData->lvList.lh_Head;
            if (workNode){
                while (nextNode = workNode->ln_Succ){
                    FreeMem(workNode, sizeof(ListViewNode));
                    workNode = nextNode ;
                }
            }
            lvResetList(&lvgData->lvList) ;
        }
    }
}

ULONG lvGetItemCount(AppGadget *lvg)
{
    struct Node *n;
    ListViewGadget *lvgData = NULL;
    ULONG i=0;
    
    lvgData = (ListViewGadget *)lvg->data ;
    
    for (n = lvgData->lvList.lh_Head; n->ln_Succ; n=n->ln_Succ){
        i++;
    }
    return i;
}

char *lvGetListText(AppGadget *lvg, ULONG code)
{
    struct Node *n;
    ListViewGadget *lvgData = NULL;
    ULONG i=0;
    
    lvgData = (ListViewGadget *)lvg->data ;
    
    for (n = lvgData->lvList.lh_Head; n->ln_Succ; n=n->ln_Succ){
        if (i == code){
            return n->ln_Name ;
        }
        i++;
    }
    return NULL;
}

void *lvGetListData(AppGadget *lvg, ULONG code)
{
	struct Node *n;
    ListViewGadget *lvgData = NULL;
    ULONG i=0;
    
    lvgData = (ListViewGadget *)lvg->data ;
    
    for (n = lvgData->lvList.lh_Head; n->ln_Succ; n=n->ln_Succ){
        if (i == code){
            return ((ListViewNode *)n)->listItemData ;
        }
        i++;
    }
    return NULL;
}