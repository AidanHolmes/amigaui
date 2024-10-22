//   Copyright 2024 Aidan Holmes
//   Integer Gadget Wrapper

#ifndef __GAD_INTEGER_HDR
#define __GAD_INTEGER_HDR

#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/exec.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include "app.h"

// This function MUST be called before calling any of the functions below.
// Can be called before or after openAppWindow or addAppGadget.
//int numberInitialise(AppGadget *g);

LONG getIntegerValue(AppGadget g);

void setIntegerValue(AppGadget g, LONG num);

#endif