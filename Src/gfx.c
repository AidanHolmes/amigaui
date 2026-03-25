#include "gfx.h"
#include <graphics/gels.h>
#include <graphics/gfxmacros.h>
#include <proto/graphics.h>
#include <exec/memory.h>
#include <string.h>

extern struct GfxBase *GfxBase ;

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

void v36FreeBitMap(struct BitMap *bmp, UWORD Width, UWORD Height)
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

void swapBitMapPlane(struct BitMap *bmp, UBYTE from, UBYTE to)
{
	PLANEPTR tmp ;
	if (bmp && from < 8 && to < 8){
		if (from != to && from < (1 << bmp->Depth) && to < (1 << bmp->Depth)){
			tmp = bmp->Planes[from];
			bmp->Planes[from] = bmp->Planes[to];
			bmp->Planes[to] = tmp;
		}
	}
}

struct BitMap* v36AllocBitMap(UWORD Width, UWORD Height, UBYTE Bitplanes)
{
	struct BitMap *bmp = NULL ;
	UWORD i=0, additionalBitplanes = 0;
	
	if (Bitplanes > 8){
		additionalBitplanes = (sizeof(PLANEPTR) * (Bitplanes - 8));
	}
	bmp = AllocVec(sizeof(struct BitMap) + additionalBitplanes, MEMF_ANY | MEMF_CLEAR);
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
		v36FreeBitMap(bmp, Width, Height);
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
	if (!(pixImg->hdr.pens = AllocVec(sizeof(LONG)*2, MEMF_ANY))){
		FreeVec(pixImg);
		return NULL; // no memory
	}
	pixImg->hdr.pens[0] = colour0;
	pixImg->hdr.pens[1] = colour1;
	pixImg->hdr.pencount = 2;
	pixImg->hdr.width = width;
	pixImg->hdr.height = height;
	pixImg->stride = (((width+15)>>4)<<4); // Align to WORD
	if (!(pixImg->pixelArray=AllocVec((pixImg->stride * height), MEMF_ANY | MEMF_CLEAR))){
		// Failed to allocate memory
		FreeVec(pixImg->hdr.pens);
		FreeVec(pixImg);
		return NULL;
	}
	for (y=0,row=pixImg->pixelArray; y < height; y++, row+=pixImg->stride){
		p=row; // p points to start of new image row
		q = xbm + (xbmstride*y); // q points to start of xmb image row
		
		for (x=0;x<width;x++){ // iterate the new image column pixels
			if (q[x>>3] & (1 << (x&0x07))){
				*p++ = pixImg->hdr.pens[1];
			}else{
				*p++ = pixImg->hdr.pens[0];
			}
		}
	}
	return pixImg;
}

__INLINE__ static UBYTE reverseBits(UBYTE b)
{
	UBYTE rev =0, i=0;
	for(;i < 8; i++){
		if (b & (1 << i)){
			rev |= (0x80 >> i);
		}
	}
	return rev;
}

// Load an XBM style image into a BitMap
// Free with freeXbmBitMap
struct BitMapImage* xbmToBitMap(UBYTE *xbm, UBYTE pen, UWORD width, UWORD height)
{
	struct BitMapImage *bmpImg;
	UBYTE *p = NULL, *q=NULL, *mask = NULL;
	UWORD xbmstride = 0, row = 0, col = 0, depth = 1;

	// depth now specifies the number of bits required to store colours - up to 8 bits (ignore more pens after this)
	xbmstride = (width+7) / 8 ; // byte align XBM
	
	if (!(bmpImg=AllocVec(sizeof(struct BitMapImage), MEMF_ANY | MEMF_CLEAR))){
		return NULL;
	}
	
	if (pen > 0){
		for (depth=0; ((0x80 & (pen<<depth)) == 0) && depth < 8;depth++);
		depth = (8 - depth) ;
	}
	if (depth < 8){ // Cannot support a mask for 8 deep colour images (we didn't allocate the additional mask layer)
		bmpImg->hasMask = TRUE ;
	}
	
	if (!(bmpImg->bmp = v36AllocBitMap(width, height, bmpImg->hasMask?2:1))){
		FreeVec(bmpImg);
		return NULL;
	}
	bmpImg->bmp->Depth = depth; 
	
	bmpImg->hdr.width = width;
	bmpImg->hdr.height = height;
	
	q = xbm;
	for(row=0; row < height;row++){
		if (bmpImg->hasMask){
			mask = bmpImg->bmp->Planes[1] + (row *bmpImg->bmp->BytesPerRow);
		}
		p = bmpImg->bmp->Planes[0] + (row *bmpImg->bmp->BytesPerRow);
		for(col=0;col < xbmstride;col++){
			*p = reverseBits(*q++);
			if (bmpImg->hasMask){
				*mask++ = ~(*p++);
			}
		}
	}
	// Arrange into pseudo multidepth to set pen
	// Remember plane 0 and mask plane 1
	bmpImg->SimplePlane = bmpImg->bmp->Planes[0];
	if (bmpImg->hasMask){
		bmpImg->MaskPlane = bmpImg->bmp->Planes[1];
	}
	if (pen > 0){
		for (depth=0; depth < bmpImg->bmp->Depth;depth++){
			if (pen & (1 << depth)){
				bmpImg->bmp->Planes[depth] = bmpImg->SimplePlane;
			}else{
				bmpImg->bmp->Planes[depth] = NULL ;
			}
		}
	}
	if (bmpImg->hasMask){
		bmpImg->bmp->Planes[depth] = bmpImg->MaskPlane;
	}
	
	return bmpImg;
}

void freeXbmBitMap(struct BitMapImage *xbmBitMap)
{
	if (xbmBitMap->SimplePlane){
		xbmBitMap->bmp->Planes[0] = xbmBitMap->SimplePlane;
		if (xbmBitMap->hasMask){
			xbmBitMap->bmp->Planes[1] = xbmBitMap->MaskPlane;
		}
		xbmBitMap->bmp->Depth = 1;
	}
	if (xbmBitMap->hasMask){
		xbmBitMap->bmp->Depth += 1;
	}

	v36FreeBitMap(xbmBitMap->bmp, xbmBitMap->hdr.width, xbmBitMap->hdr.height);
	FreeVec(xbmBitMap);
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
				case 0:pi->hdr.width = (UWORD)tmp;break;
				case 1:pi->hdr.height = (UWORD)tmp;break;
				case 2:pi->hdr.pencount = (UBYTE)tmp;break;
				case 3:pi->hdr.charspercolour = (UBYTE)tmp;break;
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
	//struct Library *GfxBase = NULL ;
		
	//if (!(GfxBase = OpenLibrary("graphic.library", 0))){
	//	return FALSE;
	//}
	
	if (pi->hdr.pencount == 0 || pi->hdr.charspercolour == 0 || pi->hdr.charspercolour > 5){
		return FALSE ; // nonsense parameters or cannot support
	}
	
	//pi->colourIDs[pi->hdr.charspercolour] = '\0'; // add terminator at fixed length
	
	for (ci=0;ci < pi->hdr.pencount; ci++){
		idi = 0;
		idc = 0;
		colour = 0;
		hexcount = 0;
		cid = pi->hdr.colourIDs+(ci*pi->hdr.charspercolour+1);
		cid[pi->hdr.charspercolour] = '\0'; // Terminate ID
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
						if (idi < pi->hdr.charspercolour){
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
		pi->hdr.colourTable[ci] = colour ;
		// Assign pen for colour
		if (cm){
			pi->hdr.pens[ci] = ObtainBestPen(cm, (colour << 0) | 0x00FFFFFF, (colour << 8) | 0x00FFFFFF, (colour << 16) | 0x00FFFFFF, OBP_Precision, PRECISION_EXACT, 0);
		}
	}
	
	ret = TRUE ;
exit:
	//CloseLibrary(GfxBase);
	return TRUE ;
}

// Update the image with new pens (redefine in PixelImage->pens array)
// This will rewrite the pixel array, retaining the same width and height of the image. Only colours are changed in the process
BOOL xpmUpdateColours(struct PixelImage *pi, UBYTE **xpm)
{
	UBYTE *row = NULL, *p = NULL, *q = NULL, writePen = 0;
	UWORD x=0, y=0, ci = 0, col = 0;
	
	for (y=0,row=pi->pixelArray; y < pi->hdr.height; y++, row+=pi->stride){
		p=row; // p points to start of new image row
		q = xpm[pi->hdr.pencount+1+y]; // q points to start of xmb image row (these appear after params and all colour strings)
		
		for (x=0;x<pi->hdr.width;x++){ // iterate the new image column pixels
			writePen = 0;
			for(col=0; col < pi->hdr.pencount; col++){ // search for pen associated with colour id
				for(ci = 0;ci < pi->hdr.charspercolour; ci++){
					if (pi->hdr.colourIDs[((pi->hdr.charspercolour+1)*col)+ci] != q[ci]){
						break;
					}
				}
				if (ci == pi->hdr.pencount){ // found a pen
					writePen = pi->hdr.pens[col];
					break ; // done, exit search
				}
			}
			q += pi->hdr.charspercolour; // jump to next colour ID
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
	if (!(pixImg->hdr.pens = AllocVec(sizeof(LONG)*pixImg->hdr.pencount, MEMF_ANY))){
		goto exit; // no memory
	}
	
	// Allocate list of pen identifers from the XPM colour table. This is only useful for processing colour information 
	// and allocating the initial pens. 
	if (!(pixImg->hdr.colourIDs = AllocVec((pixImg->hdr.charspercolour+1)*pixImg->hdr.pencount, MEMF_ANY | MEMF_CLEAR))){
		goto exit; // no memory
	}
	
	if (!(pixImg->hdr.colourTable = AllocVec(sizeof(ULONG)*pixImg->hdr.pencount, MEMF_ANY | MEMF_CLEAR))){
		goto exit; // no memory
	}
	
	// Read colours from the XPM colour array. Allocate pens using best or closest match of colours. 
	if (!_parseXPMColours(xpm, pixImg, cm)){
		goto exit;
	}
	
	// Allocate the pixel array
	pixImg->stride = (((pixImg->hdr.width+15)>>4)<<4); // Align to WORD
	if (!(pixImg->pixelArray=AllocVec((pixImg->stride * pixImg->hdr.height), MEMF_ANY | MEMF_CLEAR))){
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
		if (pi->hdr.pens){
			FreeVec(pi->hdr.pens);
			pi->hdr.pens = NULL;
		}
		if (pi->hdr.colourIDs){
			FreeVec(pi->hdr.colourIDs);
			pi->hdr.colourIDs = NULL;
		}
		if (pi->hdr.colourTable){
			FreeVec(pi->hdr.colourTable);
			pi->hdr.colourTable = NULL;
		}
		FreeVec(pi);
	}
}
