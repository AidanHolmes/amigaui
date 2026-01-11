//   Copyright 2024 Aidan Holmes
//   Integer Gadget Wrapper

#include "intgad.h"
/*
int integerInitialise(AppGadget *g)
{
	return 0;
}
*/

LONG getIntegerValue(AppGadget g)
{
	struct StringInfo *si = (struct StringInfo*)(g.gadget->SpecialInfo);
	return si->LongInt ;
}

void setIntegerValue(AppGadget g, LONG num)
{
	if (g.wnd){
		GT_SetGadgetAttrs(g.gadget, g.wnd->appWindow, NULL, GTIN_Number, (ULONG)num, TAG_END);
    }
}