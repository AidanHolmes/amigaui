//   Copyright 2024 Aidan Holmes
//   String Gadget Wrapper

#ifndef __GAD_STRING_HDR
#define __GAD_STRING_HDR

#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/exec.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include "app.h"

// This function MUST be called before calling any of the functions below.
// Can be called before or after openAppWindow or addAppGadget.
//int stringInitialise(AppGadget *g);

UBYTE *getStringValue(AppGadget g);

void setStringValue(AppGadget g, UBYTE *szStr);

#endif