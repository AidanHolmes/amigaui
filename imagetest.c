//   Copyright 2024 Aidan Holmes
//   Example using the IFF parser and image rendering

#include "iff.h"
#include <exec/types.h>
#include <stdio.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include "app.h"
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>

void window_sigs (struct App *myApp, ULONG sigs)
{
	myApp->closedispatch = TRUE ;
}

int main(int argc, char **argv)
{
	App myApp ;
	Wnd *appWnd;
	//Wnd *appWnd;
	FILE *f = NULL;
	struct IFFgfx gfx;
	UWORD ret = IFF_NO_ERROR;
	struct Screen *screen ;
	struct BitMap *img, *rtgimg;
	struct ColorSpec *cs = NULL, *pcs = NULL;
	ULONG tags[] = {SA_Colors,0,SA_Depth,8,TAG_END};
	ULONG tagsrtg[] = {SA_DisplayID,0,TAG_END};
	ULONG wndTags[] = {WA_AutoAdjust,TRUE,TAG_END};
	ULONG displaytags[] = {CYBRBIDTG_Depth,24,CYBRBIDTG_NominalWidth,640,CYBRBIDTG_NominalHeight,512};
	ULONG modeID = 0, cgfxDepth, cgfxMode, isRTG;
	struct Library		*CyberGfxBase;
	struct IFFRenderInfo ri;
	struct IFFChunkData cmap, body;
	
	if (argc < 2){
		printf("Please provide an image file name\n");
		return 5;
	}
	
	if (initialiseIFF(&gfx) != IFF_NO_ERROR){
		printf("Failed to initialise IFF\n");
		return 5;
	}
	
	f=fopen(argv[1], "r");
	if (!f){
		printf("Failed to open file %s\n", argv[1]);
		return 5;
	}
	
	initialiseApp(&myApp);
	
	myApp.wake_sigs = SIGBREAKF_CTRL_C;
	myApp.fn_wakeSigs = window_sigs;
	
	if ((ret=parseIFFImage(&gfx, f)) != IFF_NO_ERROR){
		printf("Failed to parse file stream, error %u\n", ret);
	}else{
		printf("IFF structure size %u\n", gfx.size) ;
		printf("Image width %u, height %u\n", gfx.bitmaphdr.Width,gfx.bitmaphdr.Height);
		printf("Image depth %u, masking 0x%02X, compression 0x%02X\n", gfx.bitmaphdr.Bitplanes, gfx.bitmaphdr.Masking, gfx.bitmaphdr.Compress);
		printf("CAMG flags 0x%04X\n", gfx.camg_flags);
		printf("CMAP at offset %u, length %u\n", gfx.cmap.offset, gfx.cmap.length);
		printf("BODY at offset %u, length %u\n", gfx.body.offset, gfx.body.length);
		cmap.length = gfx.cmap.length;
		cmap.offset = gfx.cmap.offset;
		body.length = gfx.body.length;
		body.offset = gfx.body.offset;
		
		appWnd = getAppWnd(&myApp);
		
		if (myApp.isScreenRTG){

			if(CyberGfxBase=OpenLibrary("cybergraphics.library",41L)){
				modeID = BestCModeIDTagList((struct TagItem*)displaytags);
				
				tagsrtg[1] = modeID;
				createAppScreen(&myApp, TRUE, TRUE, tagsrtg);

				isRTG = GetCyberMapAttr(myApp.appScreen->RastPort.BitMap, CYBRMATTR_ISCYBERGFX);
				cgfxDepth = GetCyberMapAttr(myApp.appScreen->RastPort.BitMap, CYBRMATTR_DEPTH);
				cgfxMode = GetCyberMapAttr(myApp.appScreen->RastPort.BitMap, CYBRMATTR_PIXFMT);
				printf("RTG isRTG %u, depth %u, mode %u\n", isRTG, cgfxDepth, cgfxMode);
				
				CloseLibrary(CyberGfxBase);
			}
			
			appWnd->info.Flags = WFLG_DRAGBAR |WFLG_CLOSEGADGET ;
		}else{
			//cs = createColorMap(&gfx.ctx,&cmap,4);
			//if (cs){
				//tags[1] = (ULONG)cs;
				if(myApp.isScreenRTG){
					tags[3] = 8;
				}else if(gfxChipSet() == GFX_IS_AGA){
					tags[3] = 8;
				}else{
					tags[3] = 5;
				}
				//tags[3] = gfx.bitmaphdr.Bitplanes;
				createAppScreen(&myApp, TRUE, TRUE, tags);
				//freeColorMap(cs);
			//}
			appWnd->info.Flags = WFLG_BORDERLESS | WFLG_ACTIVATE ;
		}

		screen = myApp.appScreen;
		if (screen){
			//if (myApp.isScreenRTG){
				//wndSetSize(appWnd, 640, 512);
			//}else{
				wndSetSize(appWnd, screen->Width, screen->Height);
			//}
			
			openAppWindow(appWnd, wndTags);
			printf("Opened window width %u, height %u\n", appWnd->appWindow->Width, appWnd->appWindow->Height);
			
			//img = createBitMap(&gfx.ctx, &body, &gfx.bitmaphdr, appWnd->appWindow->RPort->BitMap);
			img = createBitMap(&gfx.ctx, &body, &gfx.bitmaphdr, NULL);
			if (img){
				if (myApp.isScreenRTG){
					cs = createColorMap(&gfx.ctx,&cmap,8);
					if(cs){
						convertPlanarTo24bitRender(&ri,gfx.bitmaphdr.Width,gfx.bitmaphdr.Height,img, cs);
						freeColorMap(cs);
						if (ri.Memory){
							if(CyberGfxBase=OpenLibrary("cybergraphics.library",41L)){
								WritePixelArray(ri.Memory, 0, 0,
													ri.BytesPerRow,
													appWnd->appWindow->RPort,
													appWnd->appWindow->LeftEdge,
													appWnd->appWindow->TopEdge,
													gfx.bitmaphdr.Width,gfx.bitmaphdr.Height,
													ri.RGBFormat);
								FreeVec(ri.Memory);
								CloseLibrary(CyberGfxBase);
							}
						}
					}
					
				}else{
					setViewPortColorMap(&appWnd->appWindow->WScreen->ViewPort, &gfx.ctx, &cmap, 8);
					BltBitMap(img, 0,0,appWnd->appWindow->RPort->BitMap,appWnd->appWindow->LeftEdge + appWnd->appWindow->BorderLeft,appWnd->appWindow->TopEdge + appWnd->appWindow->BorderTop,gfx.bitmaphdr.Width,gfx.bitmaphdr.Height,0xC0,0xFF,NULL);
				}
				freeBitMap(img);
			}else{
				printf("Failed to create a new bit map from IFF\n");
			}
		}else{
			printf("Cannot lock/obtain screen\n");
		}
	}
	fclose(f);
	
	dispatch(&myApp);
    
    appCleanUp(&myApp);
	
	return 0;	
}