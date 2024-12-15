//   Copyright 2024 Aidan Holmes
//   Text Gadget Wrapper

#include "txtgad.h"
/*
int stringInitialise(AppGadget *g)
{
	return 0;
}
*/

UBYTE *getTextValue(AppGadget g)
{
	struct StringInfo *si = (struct StringInfo*)(g.gadget->SpecialInfo);
	return si->Buffer ;
}

void setTextValue(AppGadget g, UBYTE *szStr)
{

	if (g.wnd){
		GT_SetGadgetAttrs(g.gadget, g.wnd->appWindow, NULL, GTTX_Text, (ULONG)szStr, TAG_END);
    }
}