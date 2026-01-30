//   Copyright 2024 Aidan Holmes
//   String Gadget Wrapper

#include "stringgad.h"
/*
int stringInitialise(AppGadget *g)
{
	return 0;
}
*/

UBYTE *getStringValue(AppGadget g)
{
	struct StringInfo *si = (struct StringInfo*)(g.gadget->SpecialInfo);
	return si->Buffer ;
}

void setStringValue(AppGadget g, UBYTE *szStr)
{
	struct Library *GadToolsBase = NULL;
	
	if (g.wnd){
		GadToolsBase = g.wnd->app->gadtools;
		GT_SetGadgetAttrs(g.gadget, g.wnd->appWindow, NULL, GTST_String, (ULONG)szStr, TAG_END);
    }
}