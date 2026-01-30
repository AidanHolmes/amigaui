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