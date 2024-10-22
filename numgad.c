//   Copyright 2024 Aidan Holmes
//   Number Gadget Wrapper

#include "numgad.h"
/*
int numberInitialise(AppGadget *g)
{
	return 0;
}
*/

LONG getNumberValue(AppGadget g)
{
	struct StringInfo *si = (struct StringInfo*)(g.gadget->SpecialInfo);
	return si->LongInt ;
}

void setNumberValue(AppGadget g, LONG num)
{
	if (g.wnd){
		GT_SetGadgetAttrs(g.gadget, g.wnd->appWindow, NULL, GTNM_Number, (ULONG)num, TAG_END);
    }
}