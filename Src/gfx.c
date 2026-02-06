#include "gfx.h"
#include <graphics/gels.h>
#include <graphics/gfxmacros.h>
#include <proto/graphics.h>
#include <exec/memory.h>

static void _freeBob(struct GfxGelSys *sys, struct GfxBobs *pBob)
{
	if (pBob){
		if (pBob->bob.SaveBuffer){
			FreeVec(pBob->bob.SaveBuffer);
			pBob->bob.SaveBuffer = NULL;
			pBob->bob.DBuffer = NULL;
		}
		if (pBob->bob.BobVSprite && pBob->bob.BobVSprite->BorderLine){
			FreeVec(pBob->bob.BobVSprite->BorderLine);
			pBob->bob.BobVSprite->BorderLine = NULL ;
			pBob->bob.BobVSprite->CollMask = NULL ;
		}
		if (pBob->bob.DBuffer){
			FreeVec (pBob->bob.DBuffer);
			pBob->bob.DBuffer = NULL ;
		}
		if (pBob->prev){
			pBob->prev->next = pBob->next;
		}
		if (pBob->next){
			pBob->next->prev = pBob->prev;
		}
		
		if (pBob == sys->headBob){
			sys->headBob = pBob->next ; // could be null
		}
		FreeVec(pBob);
	}
}

struct GfxBobs *createBob(Wnd *pWnd, struct GfxGelSys *sys, struct VSprite *vs, UWORD screenDepth, BOOL dblBuffer)
{
	ULONG lineSize = 0, planeSize =0, rasterSize = 0, structSizes = 0;
	struct GfxBobs *newBob = NULL, *b = NULL ;
	BOOL bRet = FALSE ;
	struct Library *GfxBase = pWnd->app->gfx;
	
	lineSize = sizeof(WORD) * vs->Width;
	planeSize = lineSize * vs->Height;
	
	structSizes = sizeof(struct GfxBobs);
	if (dblBuffer){
		structSizes += sizeof(struct DBufPacket);
	}
	// Allocate memory for the Bob struct and associated ANY memory buffers
	if (!(newBob = AllocVec(structSizes, MEMF_CLEAR | MEMF_ANY))){
		return NULL;
	}
	
	rasterSize = sizeof(WORD) * vs->Width * vs->Height * screenDepth;
	// Allocate chip mem for the screen raster buffers
	if (!(newBob->bob.SaveBuffer = AllocVec(rasterSize, MEMF_CHIP))){
		goto cleanup ;
	}
	if (dblBuffer){
		newBob->bob.DBuffer = (struct DBufPacket*)((UBYTE*)newBob+sizeof(struct GfxBobs));
		// Needs allocating separately from SaveBuffer (otherwise it will crash - possible 8 byte alignment issue)
		if (!(newBob->bob.DBuffer->BufBuffer = AllocVec(rasterSize, MEMF_CHIP))){
			goto cleanup;
		}
	}
	
	// Allocate sprite
	// use the same allocation for all vsprite memory
	vs->BorderLine = (WORD*)AllocVec(lineSize+planeSize, MEMF_CHIP);
	if (!vs->BorderLine){
		goto cleanup;
	}
	vs->CollMask = vs->BorderLine + vs->Width; 
	vs->VSBob = &newBob->bob;
	InitMasks(vs); // assume we do this after setting VSBob
	
	// Complete Bob setup
	newBob->bob.BobVSprite = vs; 
	newBob->bob.ImageShadow = vs->CollMask;
	newBob->bob.Flags = 0;
	newBob->bob.Before = NULL;
	newBob->bob.After = NULL ;
	newBob->bob.BobComp = NULL;
	
	if (sys->headBob){
		for (b=sys->headBob; b->next; b = b->next);
		b->next = newBob;
	}else{
		sys->headBob = newBob;
	}
	newBob->next = NULL;
		
	AddBob(&newBob->bob, pWnd->appWindow->RPort);
	
	bRet = TRUE ;
cleanup:
	if (!bRet && newBob){
		_freeBob(sys, newBob);
		newBob = NULL ;
	}
	return newBob;
}

void removeBobs(Wnd *pWnd, struct GfxGelSys *sys)
{
	struct GfxBobs *b, *tb;
	struct Library *GfxBase = pWnd->app->gfx;
	struct Library *IntuitionBase = pWnd->app->intu;
	
	if (!pWnd->appWindow){
		return;
	}

	if (!pWnd->appWindow->RPort->GelsInfo){
		return;
	}
	if (!sys){
		return ;
	}
	if (sys->headBob){
		for (b=sys->headBob; b; b = b->next){
			RemBob(&b->bob); // flag for removal
		}
		// Check that the bobs have been removed from the gels list
		while (TRUE){// Run twice for double buffered bobs
			SortGList(pWnd->appWindow->RPort);
			DrawGList(pWnd->appWindow->RPort, ViewPortAddress(pWnd->appWindow));
			if (pWnd->appWindow->RPort->GelsInfo->gelHead->NextVSprite == pWnd->appWindow->RPort->GelsInfo->gelTail){
				break;
			}
		}
		
		WaitTOF() ;
		
		for (b=sys->headBob; b; ){
			tb = b;
			b=b->next;
			_freeBob(sys, tb); // free memory
		}
		sys->headBob = NULL ;
	}
}

BOOL initialiseGelSys(Wnd *pWnd)
{
	struct GelsInfo *gInfo = NULL ;
	struct VSprite  *vsHead = NULL;
	struct VSprite  *vsTail = NULL;
	BOOL bRet = FALSE ;
	struct Library *GfxBase = pWnd->app->gfx;
	
	
	if (!pWnd->appWindow){
		return FALSE;
	}
	if (!(gInfo = pWnd->appWindow->RPort->GelsInfo = AllocVec(sizeof(struct GelsInfo), MEMF_ANY | MEMF_CLEAR))){
		return FALSE ;
	}
	if (!(gInfo->nextLine = (WORD*)AllocVec(sizeof(WORD)*8, MEMF_ANY | MEMF_CLEAR))){
		goto cleanup;
	}
	
	if (!(gInfo->lastColor = (WORD**)AllocVec(sizeof(LONG) * 8, MEMF_ANY | MEMF_CLEAR))){
		goto cleanup;
	}
	
	if (!(gInfo->collHandler = (struct collTable *)AllocVec(sizeof(struct collTable), MEMF_ANY | MEMF_CLEAR))){
		goto cleanup;
	}
	
	if (!(vsHead = (struct VSprite *)AllocVec(sizeof(struct VSprite), MEMF_ANY | MEMF_CLEAR))){
		goto cleanup;
	}
	
	if (!(vsTail = (struct VSprite *)AllocVec(sizeof(struct VSprite), MEMF_ANY | MEMF_CLEAR))){
		goto cleanup;
	}
	
	gInfo->sprRsrvd 	= 0x03;
	gInfo->leftmost   	= gInfo->topmost    = 1;
	gInfo->rightmost  	= (pWnd->appWindow->RPort->BitMap->BytesPerRow << 3) - 1;
	gInfo->bottommost 	= pWnd->appWindow->RPort->BitMap->Rows - 1;
	
	InitGels(vsHead, vsTail, gInfo);
	
	bRet = TRUE;
cleanup:
	if (!bRet){
		cleanupGelSys(pWnd);
		if (vsHead){
			FreeVec(vsHead);
		}
		if (vsTail){
			FreeVec(vsTail);
		}
	}
	return bRet;
}

void cleanupGelSys(Wnd *pWnd)
{
	struct GelsInfo *gInfo = NULL ;
	
	if (!pWnd || !pWnd->appWindow){
		return; // no window
	}
	if (pWnd->appWindow->RPort->GelsInfo){
		gInfo = pWnd->appWindow->RPort->GelsInfo;
		
		if (gInfo->collHandler){
			FreeVec(gInfo->collHandler);
			gInfo->collHandler = NULL ;
		}
		if (gInfo->lastColor){
			FreeVec(gInfo->lastColor);
			gInfo->lastColor = NULL ;
		}
		if (gInfo->nextLine){
			FreeVec(gInfo->nextLine);
			gInfo->nextLine = NULL ;
		}
		if (gInfo->gelHead){
			FreeVec(gInfo->gelHead);
			gInfo->gelHead = NULL ;
		}
		if (gInfo->gelTail){
			FreeVec(gInfo->gelTail);
			gInfo->gelTail = NULL ;
		}
		FreeVec(gInfo);
		
		pWnd->appWindow->RPort->GelsInfo = NULL ;
	}
}

void v36FreeBitMap(struct BitMap *bmp, UWORD Width, UWORD Height, struct Library *GfxBase)
{
	UWORD i=0;
	
	if (!bmp){
		return ;
	}
	for (i=0;i<bmp->Depth;i++){
		if (bmp->Planes[i]){
			FreeRaster(bmp->Planes[i], Width, Height);
		}
	}
	FreeVec(bmp);
}

struct BitMap* v36AllocBitMap(UWORD Width, UWORD Height, UBYTE Bitplanes, struct Library *GfxBase)
{
	struct BitMap *bmp = NULL ;
	UWORD i=0;
	
	// May not need to worry about 8 byte alignment here. These are non-AGA systems below v39
	bmp = AllocVec(sizeof(struct BitMap), MEMF_ANY | MEMF_CLEAR);
	if (!bmp){
		goto cleanup;
	}
	
	InitBitMap(bmp, Bitplanes, Width, Height);
	for (i=0;i<Bitplanes;i++){
		if (bmp->Planes[i] = (PLANEPTR)AllocRaster(Width,Height)){
			BltClear(bmp->Planes[i], bmp->BytesPerRow*Height, 1); 
		}else{
			goto cleanup;
		}
	}

	return bmp;
cleanup:
	if (bmp){
		v36FreeBitMap(bmp, Width, Height, GfxBase);
	}
	return NULL;
}

// Load an XBM style image and set the 2 colours
// A new PixelImage will be created, this must be freed with freePixelImage when finished using.
struct PixelImage* xbmToPixelImage(UBYTE *xbm, UWORD width, UWORD height, UBYTE colour0, UBYTE colour1)
{
	struct PixelImage *pixImg;
	UBYTE *row = NULL, *p = NULL, *q = NULL;
	UWORD xbmstride = 0;
	UWORD x=0, y=0;

	// depth now specifies the number of bits required to store colours - up to 8 bits (ignore more pens after this)
	xbmstride = (width+7) / 8 ; // byte align XBM
	
	if (!(pixImg = AllocVec(sizeof(struct PixelImage), MEMF_ANY | MEMF_CLEAR))){
		return NULL; // no memory
	}
	if (!(pixImg->pens = AllocVec(sizeof(LONG)*2, MEMF_ANY))){
		FreeVec(pixImg);
		return NULL; // no memory
	}
	pixImg->pens[0] = colour0;
	pixImg->pens[1] = colour1;
	pixImg->pencount = 2;
	pixImg->width = width;
	pixImg->height = height;
	pixImg->stride = (((width+15)>>4)<<4); // Align to WORD
	if (!(pixImg->pixelArray=AllocVec((pixImg->stride * height), MEMF_ANY | MEMF_CLEAR))){
		// Failed to allocate memory
		FreeVec(pixImg->pens);
		FreeVec(pixImg);
		return NULL;
	}
	for (y=0,row=pixImg->pixelArray; y < height; y++, row+=pixImg->stride){
		p=row; // p points to start of new image row
		q = xbm + (xbmstride*y); // q points to start of xmb image row
		
		for (x=0;x<width;x++){ // iterate the new image column pixels
			if (q[x>>3] & (1 << (x&0x07))){
				*p++ = pixImg->pens[1];
			}else{
				*p++ = pixImg->pens[0];
			}
		}
	}
	return pixImg;
}

static LONG ASCII_To_Long(char *szNum)
{
	LONG num=0;
	BOOL neg = FALSE, reading =FALSE;
	char *p = szNum;
	for (;*p;p++){
		switch(*p){
			case ' ':case '\t':case '\n':case '\r':
				//ignore at start of parse
				if (reading){
					goto exit ; // white space indicates end of number
				}
				break;
			case '-':
				neg = TRUE;
				reading = TRUE ;
				break;
			default:
				if (*p >= '0' && *p <= '9'){
					reading = TRUE ;
					num *= 10;
					num += *p - '0';
				}else{
					if (reading){
						goto exit; // unexpected char, exit
					}
				}
		}
	}

exit:
	if (neg){
		num *= -1;
	}
	return num;
}

static BOOL _parseXPMAttribs(UBYTE **xpm, struct PixelImage *pi)
{
	char *p = NULL, *q = NULL ;
	LONG tmp = 0;
	BOOL spc = FALSE ;
	UBYTE parsingparam = 0;
	// Parse image parameters
	for (p=xpm[0]; *p != '\0';){
		spc = FALSE;
		for (q=p; *p; p++){
			if (*p == ' '){
				spc = TRUE;
				break;
			}
		}
		if (p > q){
			tmp =ASCII_To_Long(q);
			if (tmp <=0){
				return FALSE;
			}
			switch(parsingparam++){
				case 0:pi->width = (UWORD)tmp;break;
				case 1:pi->height = (UWORD)tmp;break;
				case 2:pi->pencount = (UBYTE)tmp;break;
				case 3:pi->charspercolour = (UBYTE)tmp;break;
				default:
					break;
			}
		}
		if (spc){
			*p++ = ' ' ; // restore space char
			q = p;
		}
	}
}

static BOOL _parseXPMColours(UBYTE **xpm, struct PixelImage *pi, struct ColorMap *cm)
{
	char *p = NULL, spec = 'c', *cid = NULL ; // generous support for 5 chars per colour (which is more than the 3 really needed)
	UBYTE ci = 0, idi=0, idc=0, parsingparam = 0, hexcount = 0;
	ULONG colour = 0;
	BOOL ret = FALSE;
	struct Library *GfxBase = NULL ;
		
	if (!(GfxBase = OpenLibrary("graphic.library", 0))){
		return FALSE;
	}
	
	if (pi->pencount == 0 || pi->charspercolour == 0 || pi->charspercolour > 5){
		return FALSE ; // nonsense parameters or cannot support
	}
	
	//pi->colourIDs[pi->charspercolour] = '\0'; // add terminator at fixed length
	
	for (ci=0;ci < pi->pencount; ci++){
		idi = 0;
		idc = 0;
		colour = 0;
		hexcount = 0;
		cid = pi->colourIDs+(ci*pi->charspercolour+1);
		cid[pi->charspercolour] = '\0'; // Terminate ID
		for (p = xpm[ci+1]; *p; p++){
			switch(*p){
				case ' ': case '\t': case '\n': case '\r': 
					if (idi > 0){ // started reading the identifier for colour, move to next param with whitespace
						parsingparam++;
					}
					break;
				default:
					// Any non-ws chars
					if (parsingparam == 0){ // Read the colour identifier
						if (idi < pi->charspercolour){
							cid[idi++] = *p;
						}
					}else if(parsingparam == 1){ // Read the colour spec
						spec = *p ;
					}else if(parsingparam == 2){ // Read the start of colour token
						if (*p == '#'){
							parsingparam++;
						}
					}else if(parsingparam == 3){ // Get the colour
						colour *= 16;
						if (*p >= '0' && *p <= '9'){
							colour += *p - '0';
						}else if (*p >= 'a' && *p <= 'f'){
							colour += *p - 'a' + 10;
						}else if (*p >= 'A' && *p <= 'F'){
							colour += *p - 'A' + 10;
						}else{
							goto exit; // garbage
						}
						hexcount++;
					}
			}
		}
		if (hexcount < 8){
			// Shift to align correctly (for instance; alpha byte missing)
			colour = colour << ((8-hexcount) * 4);
		}
		// Record the colour in the PixelImage
		pi->colourTable[ci] = colour ;
		// Assign pen for colour
		if (cm){
			pi->pens[ci] = ObtainBestPen(cm, (colour << 0) | 0x00FFFFFF, (colour << 8) | 0x00FFFFFF, (colour << 16) | 0x00FFFFFF, OBP_Precision, PRECISION_EXACT, 0);
		}
	}
	
	ret = TRUE ;
exit:
	CloseLibrary(GfxBase);
	return TRUE ;
}

// Update the image with new pens (redefine in PixelImage->pens array)
// This will rewrite the pixel array, retaining the same width and height of the image. Only colours are changed in the process
BOOL xpmUpdateColours(struct PixelImage *pi, UBYTE **xpm)
{
	UBYTE *row = NULL, *p = NULL, *q = NULL, writePen = 0;
	UWORD x=0, y=0, ci = 0, col = 0;
	
	for (y=0,row=pi->pixelArray; y < pi->height; y++, row+=pi->stride){
		p=row; // p points to start of new image row
		q = xpm[pi->pencount+1+y]; // q points to start of xmb image row (these appear after params and all colour strings)
		
		for (x=0;x<pi->width;x++){ // iterate the new image column pixels
			writePen = 0;
			for(col=0; col < pi->pencount; col++){ // search for pen associated with colour id
				for(ci = 0;ci < pi->charspercolour; ci++){
					if (pi->colourIDs[((pi->charspercolour+1)*col)+ci] != q[ci]){
						break;
					}
				}
				if (ci == pi->pencount){ // found a pen
					writePen = pi->pens[col];
					break ; // done, exit search
				}
			}
			q += pi->charspercolour; // jump to next colour ID
			*p++ = writePen ; // write pen ID into byte and move to next pixel in array
		}
	}
	
	return TRUE ;
}

// Convert an xpm header structure to a pixel image for use with WritePixelArray8
// Colours are assigned automatically from the colour definition in the xpm. This may be approximate if no free colours
// May fail if memory cannot be allocated or failure to parse the image from header data. Assumes well defined image
// NOTE AN EXCEPTION - this only supports #RRGGBBAA or #RRGGBB colours and not named colours!
struct PixelImage* xpmToPixelImage(UBYTE **xpm, struct ColorMap *cm)
{
	struct PixelImage *pixImg;
	BOOL ret = FALSE;
	
	// Create the holding structure for the new image. 
	if (!(pixImg = AllocVec(sizeof(struct PixelImage), MEMF_ANY | MEMF_CLEAR))){
		goto exit; // no memory
	}
	
	// Read the initial attributes of the image into PixelImage struct. 
	if (!_parseXPMAttribs(xpm, pixImg)){
		goto exit;
	}
	
	// Allocate an array of pens which holds the Workbench allocated pen for each colour of the image. 
	if (!(pixImg->pens = AllocVec(sizeof(LONG)*pixImg->pencount, MEMF_ANY))){
		goto exit; // no memory
	}
	
	// Allocate list of pen identifers from the XPM colour table. This is only useful for processing colour information 
	// and allocating the initial pens. 
	if (!(pixImg->colourIDs = AllocVec((pixImg->charspercolour+1)*pixImg->pencount, MEMF_ANY | MEMF_CLEAR))){
		goto exit; // no memory
	}
	
	if (!(pixImg->colourTable = AllocVec(sizeof(ULONG)*pixImg->pencount, MEMF_ANY | MEMF_CLEAR))){
		goto exit; // no memory
	}
	
	// Read colours from the XPM colour array. Allocate pens using best or closest match of colours. 
	if (!_parseXPMColours(xpm, pixImg, cm)){
		goto exit;
	}
	
	// Allocate the pixel array
	pixImg->stride = (((pixImg->width+15)>>4)<<4); // Align to WORD
	if (!(pixImg->pixelArray=AllocVec((pixImg->stride * pixImg->height), MEMF_ANY | MEMF_CLEAR))){
		// Failed to allocate memory
		goto exit;
	}
	
	// Use the parsed and allocated colours to create the pixel array image
	if (!xpmUpdateColours(pixImg, xpm)){
		goto exit;
	}
	
	ret = TRUE ; // success
exit:
	if (!ret){
		freePixelImage(pixImg);
		pixImg = NULL;
	}
	return pixImg;
}

void freePixelImage(struct PixelImage *pi)
{
	if (pi){
		if (pi->pixelArray){
			FreeVec(pi->pixelArray);
			pi->pixelArray = NULL;
		}
		if (pi->pens){
			FreeVec(pi->pens);
			pi->pens = NULL;
		}
		if (pi->colourIDs){
			FreeVec(pi->colourIDs);
			pi->colourIDs = NULL;
		}
		if (pi->colourTable){
			FreeVec(pi->colourTable);
			pi->colourTable = NULL;
		}
		FreeVec(pi);
	}
}
