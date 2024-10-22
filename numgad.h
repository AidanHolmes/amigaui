//   Copyright 2024 Aidan Holmes
//   Number Gadget Wrapper

#ifndef __GAD_NUMBER_HDR
#define __GAD_NUMBER_HDR

#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/exec.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include "app.h"

// This function MUST be called before calling any of the functions below.
// Can be called before or after openAppWindow or addAppGadget.
//int numberInitialise(AppGadget *g);

LONG getNumberValue(AppGadget g);

void setNumberValue(AppGadget g, LONG num);

#endif