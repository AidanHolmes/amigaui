//   Copyright 2024 Aidan Holmes
//   Text Gadget Wrapper

#ifndef __GAD_TEXT_HDR
#define __GAD_TEXT_HDR

#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/exec.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include "app.h"

// This function MUST be called before calling any of the functions below.
// Can be called before or after openAppWindow or addAppGadget.
//int textInitialise(AppGadget *g);

UBYTE *getTextValue(AppGadget g);

void setTextValue(AppGadget g, UBYTE *szStr);

#endif