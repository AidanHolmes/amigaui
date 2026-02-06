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

void v36FreeBitMap(struct BitMap *bmp, UWORD Width, UWORD Height, struct Library *GfxBase);
struct BitMap* v36AllocBitMap(UWORD Width, UWORD Height, UBYTE Bitplanes, struct Library *GfxBase);

// X-Bitmap style images with some extension to support more than 2 colours

// Pixel array for use with WritePixelArray8
struct PixelImage{
	UBYTE *pixelArray;			// Allocated automatically. Pixel array which can be used in WritePixelArray8
	LONG *pens;					// Allocated automatically. Array of pens either taken from XBM creation function or from XPM attributes.
	UBYTE pencount;				// Writes the number of pens in pens array
	UBYTE charspercolour; 		// XPM only - characters used to define colour
	UBYTE *colourIDs; 			// XPM only - null terminated list of char identifiers for colours. Offset relates to pens index (e.g pen[5] identifer is related to colourIDs+(5*charspercolour))
	ULONG *colourTable;			// XPM only - array of RRGGBBAA colours that correspond to the pens array and colourIDs list
	UWORD width;				// Image width either from XBM parameter and header define or read from XPM attributes
	UWORD height;				// Image height either from XBM parameter and header define or read from XPM attributes
	UWORD stride;				// Pixel array stride (bytes per row - aligned to 16bits (WORD))
};

// Load an XBM style image and set the 2 colours
// A new PixelImage will be created, this must be freed with freePixelImage when finished using.
struct PixelImage* xbmToPixelImage(UBYTE *xbm, UWORD width, UWORD height, UBYTE colour0, UBYTE colour1);

// Convert an XBM header structure to a pixel image for use with WritePixelArray8
// Colours are assigned automatically from the colour definition in the XBM. This may be approximate if no free colours
// May fail if memory cannot be allocated or failure to parse the image from header data. Assumes well defined image
// NOTE AN EXCEPTION - this only supports #RRGGBBAA or #RRGGBB colours and not named colours!
struct PixelImage* xpmToPixelImage(UBYTE **xpm, struct ColorMap *cm);

// Update the image with new pens (redefine in PixelImage->pens array)
// This will rewrite the pixel array, retaining the same width and height of the image. Only colours are changed in the process
// Note that this will not update the colourTable and this array will retain the original image colours from XPM data
BOOL xpmUpdateColours(struct PixelImage *pi, UBYTE **xpm);

// Frees an XPM or XBM image
void freePixelImage(struct PixelImage *pi);


#endif