#ifndef __APP_GFX_HDR
#define __APP_GFX_HDR

#include "app.h"
#include <graphics/rastport.h>
#include <graphics/gels.h>

struct GfxBobs;

struct GfxBobs{
	struct GfxBobs *next;
	struct GfxBobs *prev;
	struct Bob bob;
};

struct GfxGelSys{
	struct GfxBobs *headBob;
};

// Initialise Gel System for the window. 
BOOL initialiseGelSys(Wnd *pWnd);

// MUST clean up all Gel objects before calling as 
// objects are not tracked in a Wnd
void cleanupGelSys(Wnd *pWnd);

// Create a new Bob and add to window
struct GfxBobs *createBob(Wnd *pWnd, struct GfxGelSys *sys, struct VSprite *vs, UWORD screenDepth, BOOL dblBuffer);
void removeBobs(Wnd *pWnd, struct GfxGelSys *sys);

void v36FreeBitMap(struct BitMap *bmp, UWORD Width, UWORD Height);
struct BitMap* v36AllocBitMap(UWORD Width, UWORD Height, UBYTE Bitplanes);


#endif