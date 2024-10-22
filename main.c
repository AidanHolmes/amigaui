//   Copyright 2024 Aidan Holmes
//   Example application using the amigaui library

#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <stdio.h>
#include "app.h"
#include "listgad.h"
#include "stringgad.h"
#include "txtgad.h"

struct TextAttr topaz8 = {
   (STRPTR)"topaz.font", 8, 0, 1
};

ULONG txtTags[] = {GTST_MaxChars, 200, TAG_DONE};
ULONG intTags[] = {GTNM_Border, TRUE, TAG_DONE};
ULONG listTags[] = {GTLV_ShowSelected, NULL, TAG_DONE};
AppGadget txtCtrl = {STRING_KIND, {63, 26, 172, 13, (UBYTE *)"Name", &topaz8, 1, PLACETEXT_LEFT, NULL, NULL}};
AppGadget intCtrl = {TEXT_KIND,{62, 50, 175, 15, (UBYTE *)"Output", &topaz8, 2, PLACETEXT_LEFT, NULL, NULL}};
AppGadget btnCtrl = {BUTTON_KIND,{111, 105, 54, 31, (UBYTE *)"OK", &topaz8, 3, PLACETEXT_IN, NULL, NULL}};
AppGadget btnCtrl2 = {BUTTON_KIND,{20, 20, 54, 31, (UBYTE *)"Go", &topaz8, 1, PLACETEXT_IN, NULL, NULL}};
AppGadget listCtrl = {LISTVIEW_KIND, {63, 140, 172, 50, (UBYTE *)"Ports", &topaz8, 4, PLACETEXT_LEFT, NULL, NULL}};
	
void appTick(Wnd *wnd)
{
	//printf("Tick\n");
}

void lvSelected(AppGadget *lvg, struct IntuiMessage *m)
{
	ULONG *pData = NULL;
	pData = (ULONG *)lvGetListData(lvg, m->Code) ;
	if (pData){
		printf("Entry has data %u\n", *pData) ;
	}
    printf("Entry: %s\n", lvGetListText(lvg, m->Code));
	//lvHighlightItem(lvg->wnd, lvg, m->Code);
    //lvDeleteEntry(lvg, m->Code);
    //lvUpdateList(lvg->wnd, lvg);
}

void btnClicked(AppGadget *lvg, struct IntuiMessage *m)
{
	Wnd *childWnd;
	
    printf("String buffer: %s\n", getStringValue(txtCtrl)); 
	setTextValue(intCtrl, getStringValue(txtCtrl)) ;
	setStringValue(txtCtrl, "World") ;
	
	if ((childWnd=findAppWindowName(lvg->wnd->app, "test"))){
		openAppWindow(childWnd, NULL) ;
	}
}

void btnClicked2(AppGadget *lvg, struct IntuiMessage *m)
{
	Wnd *childWnd;
	struct FileRequester *fr;
	
	if ((childWnd=findAppWindowName(lvg->wnd->app, "test"))){
		fr = openFileLoad(childWnd, "Load JPG Files", "RAM:", "#?.jpg");
		if (fr){
			printf("load file %s/%s\n",fr->fr_Drawer,fr->fr_File);
		}
	}
}

int main(void)
{
    App myApp ;
	Wnd *appWnd, *childWnd;
	ULONG lvdata[] = {123,456,789,012};
  
    setGadgetTags(&listCtrl, listTags);
    setGadgetTags(&intCtrl, intTags);
    setGadgetTags(&txtCtrl, txtTags);
	
    initialiseApp(&myApp);
	
	createAppScreen(&myApp, TRUE, TRUE, NULL); 
	
	appWnd = getAppWnd(&myApp);
	wndSetSize(appWnd, 350, 300);
	appWnd->info.Title = "Test Wnd";
    
    addAppGadget(appWnd, &txtCtrl) ;
    addAppGadget(appWnd, &intCtrl) ;
    addAppGadget(appWnd, &btnCtrl) ;
    addAppGadget(appWnd, &listCtrl);
    
    lvInitialise(&listCtrl);
    lvAddEntry(&listCtrl, -1, "One", &lvdata[0]);
    lvAddEntry(&listCtrl, -1, "Two", &lvdata[1]);
    lvAddEntry(&listCtrl, -1, "Three", &lvdata[2]);
    lvAddEntry(&listCtrl, -1, "Four", NULL);
    lvAddEntry(&listCtrl, -1, "Five", NULL);
    lvAddEntry(&listCtrl, -1, "Six", NULL);
    lvAddEntry(&listCtrl, -1, "Seven", NULL);

    lvUpdateList(appWnd, &listCtrl);
    listCtrl.fn_gadgetUp = lvSelected;
    btnCtrl.fn_gadgetUp = btnClicked;
	
	setStringValue(txtCtrl, "Hello") ;
	
	childWnd = addChildWnd(appWnd, "test", "Child");
	wndSetSize(childWnd, 450, 200);
	wndSetModal(childWnd, TRUE);
	addAppGadget(childWnd, &btnCtrl2) ;
	
	btnCtrl2.fn_gadgetUp = btnClicked2;
    
    openAppWindow(appWnd, NULL);
	
	// Set app callback
	appWnd->fn_intuiTicks = appTick;
       
    dispatch(&myApp);
    
    lvFreeList(&listCtrl);
    appCleanUp(&myApp);
    
    return(0);
 }