#include "app.h"
#include <stdio.h>
#include <string.h>
#include <proto/timer.h>
#include <devices/timer.h>
#include <exec/io.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/graphics.h>
#include <graphics/view.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <exec/libraries.h>
#include <proto/utility.h>

#define AslBase myApp->asl
#define GfxBase myApp->gfx
#define IntuitionBase myApp->intu
#define GadToolsBase myApp->gadtools
#define CyberGfxBase myApp->cgfx
#define UtilityBase myApp->util

//#define MIN_IDCMP (IDCMP_GADGETUP | IDCMP_NEWSIZE | IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW | IDCMP_CHANGEWINDOW | IDCMP_CLOSEWINDOW | LISTVIEWIDCMP | TEXTIDCMP)
#define MIN_IDCMP (IDCMP_GADGETUP | IDCMP_NEWSIZE | IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW | IDCMP_CHANGEWINDOW | IDCMP_CLOSEWINDOW | IDCMP_MENUPICK)

Wnd* _findAppWndR(Wnd *fromWnd, struct Window *find, UBYTE *strfind);

static void _showErrorRequester(App *myApp, char *Text, char *Button) 
{
	struct EasyStruct Req;

	Req.es_Title        = "App Error";
	Req.es_TextFormat   = (UBYTE*)Text;
	Req.es_GadgetFormat = (UBYTE*)Button;
	EasyRequestArgs(NULL, &Req, NULL, NULL);
}

static void _reset_list(struct List *l)
{
	l->lh_Tail = NULL;
    l->lh_Head = (struct Node *)&l->lh_Tail;
    l->lh_TailPred = (struct Node *)&l->lh_Head;
}

static ULONG _waitTimer(struct IORequest* tmr, ULONG secs, ULONG micro, ULONG sigs)
{
    ULONG sig = 1 << tmr->io_Message.mn_ReplyPort->mp_SigBit;

	if (secs > 0 || micro > 0){
		tmr->io_Command = TR_ADDREQUEST;
		((struct timerequest*)tmr)->tr_time.tv_secs = secs;
		((struct timerequest*)tmr)->tr_time.tv_micro = micro;
		
		SendIO(tmr);
		
	}else{
		sig = 0 ;
	}
	
	sigs = Wait(sigs | sig);
	if (secs > 0 || micro > 0){
		if ((sigs & sig) == 0)
		{
			if (!CheckIO(tmr)){
				AbortIO(tmr);
			}
		}
		WaitIO(tmr);
		SetSignal(0, sig);
	}
    
    return sigs ;
}

static struct IORequest* _createTimer(void)
{
    struct MsgPort *p = CreateMsgPort();
    if (p){
        struct IORequest *io = CreateIORequest(p, sizeof(struct timerequest));
        if (io){
            if (OpenDevice("timer.device", UNIT_MICROHZ, io, 0) == 0){
                return io;
            }
            DeleteIORequest(io);
        }
        DeleteMsgPort(p);
    }

    return NULL;
}

static void _closeTimer(struct IORequest *tmr)
{
	struct MsgPort *port = NULL ;
    if (tmr){
        port = tmr->io_Message.mn_ReplyPort;
        CloseDevice(tmr);
        DeleteIORequest(tmr);
        DeleteMsgPort(port);
    }
}

ULONG getScreenWidth(App *myApp)
{
	if (!myApp || !myApp->appScreen){
		return 0;
	}
	if (GfxBase->lib_Version >= 39){
		return GetBitMapAttr(myApp->appScreen->RastPort.BitMap,BMA_WIDTH);
	}
	return myApp->appScreen->Width;
}

ULONG getScreenHeight(App *myApp)
{
	if (!myApp || !myApp->appScreen){
		return 0;
	}
	if (GfxBase->lib_Version >= 39){
		return GetBitMapAttr(myApp->appScreen->RastPort.BitMap,BMA_HEIGHT);
	}
	return myApp->appScreen->Height;
}

ULONG getScreenDepth(App *myApp)
{
	if (!myApp || !myApp->appScreen){
		return 0;
	}
	if (GfxBase->lib_Version >= 39){
		return GetBitMapAttr(myApp->appScreen->RastPort.BitMap,BMA_DEPTH);
	}
	return myApp->appScreen->RastPort.BitMap->Depth;
}

UWORD gfxChipSet(void)
{
    volatile unsigned short *id = (unsigned short*)0xDFF07C;
    UBYTE i = 20;
	UWORD ret = 0, lastret = 0;
	
	for (;i >0; i--){
		switch (*id & 0x00FF){
			case 0xF8:
			ret = GFX_IS_AGA;
			break;
			case 0x00FC:
			ret = GFX_IS_ECS;
			break;
			default:
			ret = GFX_IS_OCS;
		}
		if(i == 20){
			lastret = ret;
		}else{
			if (ret != lastret){
				// changes, this must be OCS which doesn't have a register (noise in mem/bus)
				return GFX_IS_OCS;
			}
		}
	}
	
	return ret ;
}

int createAppScreen(App *myApp, BOOL hires, BOOL laced, ULONG *tags)
{
	if (myApp->appScreen){
		if (!myApp->customscreen){
			UnlockPubScreen(NULL, myApp->appScreen);
			myApp->appScreen = NULL;
			if (myApp->visual){ 
				FreeVisualInfo(myApp->visual);
				myApp->visual = NULL ;
			}
		}else{
			// what to do? We may have windows open - do nothing
			return 0;
		}
	}
	
	if (hires){
		myApp->screeninfo.ViewModes |= HIRES;
	}
	if (laced){
		myApp->screeninfo.ViewModes |= LACE;
	}
	myApp->appScreen = OpenScreenTagList(&myApp->screeninfo, (struct TagItem*)tags);
	if (!myApp->appScreen){
		_showErrorRequester(myApp, "Cannot open custom screen", "Exit");
		return 5;
	}
	if (!(myApp->visual = GetVisualInfo(myApp->appScreen,TAG_DONE))) {
		_showErrorRequester(myApp, "Failed to get visual info", "Exit");
		return(5);
	}
	if (GfxBase->lib_Version >= 39){
		if ((GetBitMapAttr(myApp->appScreen->RastPort.BitMap,BMA_FLAGS) & BMF_STANDARD) == 0){
			myApp->isScreenRTG = TRUE ; // it's a guess as this actually means that the screen is not in chip memory
		}
	}
	myApp->customscreen = TRUE ;
	
	return 0;
}

int initialiseApp(App *myApp)
{	
	int ret = 5 ;
	memset(myApp, 0, sizeof(App));
	myApp->screeninfo.Width = STDSCREENWIDTH;
	myApp->screeninfo.Height = STDSCREENHEIGHT;
	myApp->screeninfo.Depth = 3;
	myApp->screeninfo.ViewModes = 0;
	myApp->screeninfo.Type = CUSTOMSCREEN | SCREENQUIET ;
	myApp->screeninfo.DetailPen = (UBYTE)-1;
	myApp->screeninfo.BlockPen = (UBYTE)-1;
	
	if (!(myApp->tmr = _createTimer())){
		_showErrorRequester(myApp, "Failed to open timer device", "Exit");
		goto exit;
	}
	
	if (!(myApp->intu=OpenLibrary("intuition.library",36L))){
		_showErrorRequester(myApp, "Failed to open intuition library", "Exit");
		goto exit;
	}

	if (!(myApp->gadtools=OpenLibrary("gadtools.library",36L))){
		_showErrorRequester(myApp, "Failed to open gadtools library", "Exit");
		goto exit;
	}
	
	if (!(myApp->util=OpenLibrary("utility.library",37L))){
		_showErrorRequester(myApp, "Failed to open utility library", "Exit");
		goto exit;
	}
	
	if(!(myApp->gfx=OpenLibrary("graphics.library",36L))){
		_showErrorRequester(myApp, "Failed to open graphics library", "Exit");
		goto exit;
	}
	
    /* Lock screen and get visual info for gadtools */
	if (myApp->appScreen = LockPubScreen(NULL)) {
		if (!(myApp->visual = GetVisualInfo(myApp->appScreen,TAG_DONE))) {
			_showErrorRequester(myApp, "Failed to get visual info", "Exit");
			goto exit;
		}
		if (GfxBase->lib_Version >= 39){
			if ((GetBitMapAttr(myApp->appScreen->RastPort.BitMap,BMA_FLAGS) & BMF_STANDARD) == 0){
				myApp->isScreenRTG = TRUE ; // it's a guess as this actually means that the screen is not in chip memory
			}
		}
	}
	else {
		_showErrorRequester(myApp, "Failed to lock screen", "Exit");
		goto exit;
	}
	
	if (!(myApp->msgPort = CreateMsgPort())){
		_showErrorRequester(myApp, "Failed to create UI message port", "Exit");
		goto exit;
	}
	if((myApp->asl=OpenLibrary("asl.library",36L))){
		if (!(myApp->mainWnd.fileRequester = AllocAslRequest(ASL_FileRequest, NULL))){
			_showErrorRequester(myApp, "Failed to allocate ASL file requester", "Exit");
			goto exit;
		}
	}
	
	if(myApp->cgfx=OpenLibrary("cybergraphics.library",41L)){
		myApp->isScreenRTG = GetCyberMapAttr(myApp->appScreen->RastPort.BitMap, CYBRMATTR_ISCYBERGFX);
	}
	
	ret = initialiseWnd(myApp, &myApp->mainWnd, NULL) ;
exit:
	if (ret != 0){
		// Error, clean up
		appCleanUp(myApp);
	}
	return ret;
}

__INLINE__ struct Wnd* getAppWnd(App *app)
{
	return &app->mainWnd;
}

void appCleanUp(App *myApp)
{
	wndCleanUp(&myApp->mainWnd);

	if (myApp->msgPort){
		DeleteMsgPort(myApp->msgPort);
		myApp->msgPort = NULL;
	}
	if (myApp->visual){ 
		FreeVisualInfo(myApp->visual);
		myApp->visual = NULL ;
	}
	if (myApp->appScreen){
		if (!myApp->customscreen){
			UnlockPubScreen(NULL, myApp->appScreen); // should be unlocked after first openwindow call
		}else{
			// May fail - TO DO: what to do?
			if (!CloseScreen(myApp->appScreen)){
				_showErrorRequester(myApp, "Failed to close screen", "Exit");
			}
		}
		myApp->appScreen = NULL ;
	}
	if (myApp->mainWnd.fileRequester){
		FreeAslRequest(myApp->mainWnd.fileRequester);
		myApp->mainWnd.fileRequester = NULL ;
	}
	_closeTimer(myApp->tmr);
	if (myApp->cgfx){
		CloseLibrary(myApp->cgfx);
		myApp->cgfx = NULL;
	}
	if (myApp->asl){
		CloseLibrary(myApp->asl);
		myApp->asl = NULL;
	}
	if (myApp->gfx){
		CloseLibrary(myApp->gfx);
		myApp->gfx = NULL;
	}
	if (myApp->intu){
		CloseLibrary(myApp->intu);
		myApp->intu = NULL;
	}
	if (myApp->gadtools){
		CloseLibrary(myApp->gadtools);
		myApp->gadtools = NULL;
	}
	if (myApp->util){
		CloseLibrary(myApp->util);
		myApp->util = NULL;
	}
}

int initialiseWnd(App *myApp, Wnd *myWnd, UBYTE *wndTitle)
{
	myWnd->_n.ln_Name = NULL ;
	myWnd->app = myApp ;
	myWnd->lastModal = NULL ;
	myWnd->parent = NULL ;
	myWnd->modal = FALSE ;
	myWnd->idcmp = MIN_IDCMP;
    myWnd->gtail = NULL ;
    myWnd->appWindow = NULL ;
	myWnd->fn_intuiTicks = NULL;
	myWnd->fn_windowEvent = NULL;
	myWnd->fileRequester = NULL ;
	myWnd->menu = NULL ;
	myWnd->info.LeftEdge = 10;
	myWnd->info.TopEdge = 15;
	myWnd->info.Width = 350;
	myWnd->info.Height = 300;
	myWnd->info.DetailPen = (UBYTE)-1;
	myWnd->info.BlockPen = (UBYTE)-1;
	myWnd->info.IDCMPFlags = 0 ; // stop creation of port
	myWnd->info.Flags = WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_ACTIVATE | WFLG_SMART_REFRESH;
	myWnd->info.FirstGadget = NULL ; // set below
	myWnd->info.Screen = NULL;
	myWnd->info.Type = PUBLICSCREEN ;
	myWnd->info.CheckMark = NULL ;
	myWnd->info.Title = wndTitle;
	myWnd->info.BitMap = NULL ;
	myWnd->info.MinWidth = 350;
	myWnd->info.MinHeight = 300;
	myWnd->info.MaxWidth = (UWORD)~0;
	myWnd->info.MaxHeight = (UWORD)~0 ;
	_reset_list(&myWnd->childWnd);
	
	/* Create the gadget list */
	if (!(myWnd->gtail = CreateContext(&(myWnd->info.FirstGadget)))) {
		_showErrorRequester(myApp, "Failed to create gadtools context", "Exit");
		return 5;
	}
	
	return 0 ;
}

int openAppWindow(Wnd *myWnd, ULONG *tags)
{
	App *myApp = myWnd->app;
	
	if (myWnd->appWindow){
		// Already open
		return 0 ;
	}
	if (myWnd->parent){
		// Has parent, open centrally
		wndSetRelativePos(myWnd->parent, myWnd, TRUE);
	}
	if (myWnd->app->customscreen){
		myWnd->info.Screen = myWnd->app->appScreen;
		myWnd->info.Type = CUSTOMSCREEN;
	}
	
    myWnd->appWindow = OpenWindowTagList(&myWnd->info, (struct TagItem *)tags);
    if (!myWnd->appWindow){
        return (5);
	}

	// pointer to pub screen should be unlocked after first openwindow call.
	if (myWnd->app->appScreen && !myWnd->app->customscreen){
		UnlockPubScreen(NULL, myWnd->app->appScreen); 
		//myWnd->app->appScreen = NULL ;
	}
	
	// If window is modal then remember previous modal (if any) and set
	// this window as the new for the app
	if (myWnd->modal){
		myWnd->lastModal = myWnd->app->modalWnd;
		myWnd->app->modalWnd = myWnd ;
	}
	
	myWnd->appWindow->UserData = (BYTE*)myWnd ;
	myWnd->appWindow->UserPort = myWnd->app->msgPort; // reuse message port
	wndSetIDCMP(myWnd, myWnd->idcmp); // should modify IDCMP on new window
	
	ActivateWindow(myWnd->appWindow);
	WindowToFront(myWnd->appWindow);
	
    GT_RefreshWindow(myWnd->appWindow, NULL); /* Update window */
    return 0;
}

void closeAppWindow(Wnd *myWnd)
{
	struct IntuiMessage *msg = NULL;
	struct Node *n = NULL ;
	App *myApp = myWnd->app;
	
	Forbid();
	
	// If this window was the modal window and it's now
	// closed then pass back the previous modal value (or NULL)
	if (myWnd->app->modalWnd == myWnd){
		myWnd->app->modalWnd = myWnd->lastModal ;
	}
	
	if (myWnd->appWindow){
		msg = (struct IntuiMessage *)myWnd->appWindow->UserPort->mp_MsgList.lh_Head;

		// Close all pending messages
		while(n = msg->ExecMessage.mn_Node.ln_Succ){
			if (msg->IDCMPWindow == myWnd->appWindow){
				Remove((struct Node*)msg);
				ReplyMsg((struct Message*)msg);
			}
			msg = (struct IntuiMessage*)n;
		}

		myWnd->appWindow->UserPort = NULL ;
		ModifyIDCMP(myWnd->appWindow,0) ; // stop new messages
		CloseWindow(myWnd->appWindow);
		myWnd->appWindow = NULL ;
	}
	Permit();
}

void wndSetRelativePos(Wnd *primary, Wnd *secondary, BOOL centre)
{
	App *myApp = primary->app;
	
	// align top left or over the centre of primary window
	WORD xcent,ycent;
	
	if (primary->appWindow){
		primary->info.LeftEdge = primary->appWindow->LeftEdge;
		primary->info.TopEdge = primary->appWindow->TopEdge;
		primary->info.Height = primary->appWindow->Height;
		primary->info.Width = primary->appWindow->Width;
	}
	if (secondary->appWindow){
		secondary->info.LeftEdge = secondary->appWindow->LeftEdge;
		secondary->info.TopEdge = secondary->appWindow->TopEdge;
		secondary->info.Height = secondary->appWindow->Height;
		secondary->info.Width = secondary->appWindow->Width;
	}
	
	if (centre){
		xcent = primary->info.LeftEdge + (primary->info.Width / 2);
		ycent = primary->info.TopEdge + (primary->info.Height / 2);
		
		secondary->info.LeftEdge = xcent - (secondary->info.Width / 2) ;
		if (secondary->info.LeftEdge < 0){
			secondary->info.LeftEdge = 0;
		}
		secondary->info.TopEdge = ycent - (secondary->info.Height / 2) ;
		if (secondary->info.TopEdge < 0){
			secondary->info.TopEdge = 0;
		}
	}else{
		secondary->info.LeftEdge = primary->info.LeftEdge;
		secondary->info.TopEdge = primary->info.TopEdge;
	}
	
	if (secondary->appWindow){
		ChangeWindowBox(secondary->appWindow, 
						secondary->info.LeftEdge, 
						secondary->info.TopEdge, 
						secondary->info.Width, 
						secondary->info.Height);
	}
}

__INLINE__ void wndSetModal(Wnd *myWnd, BOOL modal)
{
	myWnd->modal = modal ;
}

void wndSetSize(Wnd *myWnd, WORD width, WORD height)
{
	App *myApp = myWnd->app;
	
	myWnd->info.Width = width;
	myWnd->info.Height = height;
	if (myWnd->appWindow){
		// window exists
		ChangeWindowBox(myWnd->appWindow, myWnd->appWindow->LeftEdge, myWnd->appWindow->TopEdge, width, height);
	}
}

void wndSetIDCMP(Wnd *myWnd, ULONG idcmp)
{
	App *myApp = myWnd->app;
	
	idcmp |= MIN_IDCMP; // must have something otherwise shared msgport is destroyed
	
	myWnd->idcmp = idcmp;
	if (myWnd->appWindow){
		ModifyIDCMP(myWnd->appWindow, idcmp) ;
	}
}

Wnd* addChildWnd(Wnd *wnd, UBYTE *name, UBYTE *title)
{
	Wnd *new = NULL ;
	UBYTE *str = NULL ;
	
	new = AllocVec(sizeof(Wnd), MEMF_PUBLIC | MEMF_CLEAR);
	if (!new){
		return NULL;
	}
	
	if (name){
		str = AllocVec(strlen(name)+1, MEMF_PUBLIC | MEMF_CLEAR);
		if(!str){
			FreeVec(new);
			return NULL;
		}
		strcpy(str, name);
	}
	
	AddTail(&wnd->childWnd, (struct Node *)new);
	if (initialiseWnd(wnd->app, new, title) != 0){
		Remove((struct Node *)new);
		FreeVec(new);
		FreeVec(str);
		return NULL;
	}
	new->_n.ln_Name = str ;
	new->parent = wnd ;
	return new;
}

struct Gadget* addAppGadget(Wnd *myWnd, AppGadget *gad)
{
	App *myApp = myWnd->app;
	
    gad->def.ng_VisualInfo = myWnd->app->visual;
    gad->gadget = CreateGadgetA(gad->gadgetkind, myWnd->gtail, &(gad->def), gad->gadgetTags);
    if (!gad->gadget){
        return NULL;
    }
	
    myWnd->gtail = gad->gadget;
    // Associate the gadget to the AppGadget
    gad->gadget->UserData = gad;
    gad->fn_gadgetUp = NULL;
    gad->wnd = myWnd ;
	if (myWnd->appWindow){ 
		// Already has an active window open
		RefreshGadgets(myWnd->info.FirstGadget, myWnd->appWindow, NULL);
		GT_RefreshWindow(myWnd->appWindow, NULL); // Update gadtools in window
	}		
    return gad->gadget;
}

void delAppGadget(AppGadget *gad)
{
	App *myApp = NULL;
	
	if (gad->wnd && gad->wnd->appWindow){
		myApp = gad->wnd->app;
		RemoveGadget(gad->wnd->appWindow, gad->gadget);
		//FreeGadgets(gad->gadget);
		gad->gadget = NULL ;
		RefreshGList(gad->wnd->info.FirstGadget, gad->wnd->appWindow, NULL, -1);
		GT_RefreshWindow(gad->wnd->appWindow, NULL); // Update gadtools in window
	}
}

void setGadgetTags(AppGadget *g, ULONG *tags)
{
	App *myApp = NULL;
    g->gadgetTags = (struct TagItem *)tags;
	if (g->wnd && g->wnd->appWindow){
		myApp = g->wnd->app;
		// Window exists. The gadget will need updating
		GT_SetGadgetAttrsA(g->gadget, g->wnd->appWindow, NULL, (struct TagItem *)tags) ;
	}
}

void wndCleanUp(Wnd *myWnd)
{
	struct Node *n = NULL ;
	App *myApp = myWnd->app;
	
	if (!IsListEmpty(&myWnd->childWnd)){
		while((n=RemHead(&myWnd->childWnd))){
			// recurse clean up of child windows first, then free memory
			wndCleanUp((Wnd*)n) ;
			FreeVec(n) ;
		}
	}
	
	closeAppWindow(myWnd);
	
	if (myWnd->_n.ln_Name){
		FreeVec(myWnd->_n.ln_Name);
	}
    /* Free gadgets */
    if (myWnd->info.FirstGadget){
		FreeGadgets(myWnd->info.FirstGadget);
		myWnd->info.FirstGadget = NULL ;
	}
	
	if (myWnd->menu){
		ClearMenuStrip(myWnd->appWindow);
		FreeMenus(myWnd->menu);
		myWnd->menu = NULL;
	}
}

Wnd* _findAppWndR(Wnd *fromWnd, struct Window *find, UBYTE *strfind)
{
	struct Node *n = NULL ;
	Wnd *w = NULL ;
	App *myApp = fromWnd->app;
	
	if (IsListEmpty(&fromWnd->childWnd)){
		return NULL ;
	}
	for(n=fromWnd->childWnd.lh_Head; n->ln_Succ != NULL; n = n->ln_Succ){
		w = (Wnd*)n ;
		if (find){
			if (w->appWindow == find){ // does this match?
				return w;
			}
		}
		if (strfind){
			if (w->_n.ln_Name && Stricmp(w->_n.ln_Name, strfind) == 0){
				return w;
			}
		}
		// else recurse to search further child windows
		if ((w = _findAppWndR(w,find,strfind))){
			return w;
		}
	}
	
	// exhausted search, return NULL
	return NULL;
}

Wnd* findAppWindow(App *myApp, struct Window *find)
{
	// recursive search of windows - not efficient
	return _findAppWndR(&myApp->mainWnd, find, NULL);
}

Wnd* findAppWindowName(App *myApp, UBYTE *strfind)
{
	// recursive search of windows - not efficient
	return _findAppWndR(&myApp->mainWnd, NULL, strfind);
}

struct FileRequester* openFileLoad(Wnd *parent, UBYTE *szTitle, UBYTE *szDrawer, UBYTE *szPattern)
{
	App *myApp = NULL;
	struct TagItem tags[] = {{ASLFR_Window,0},{TAG_IGNORE,0},{TAG_IGNORE,0},{TAG_IGNORE,0},{TAG_DONE,0}};
	
	if (!parent){
		return NULL;
	}
	
	myApp = parent->app;
	
	if (!myApp->asl){
		return NULL;
	}

	tags[0].ti_Data = (ULONG)parent->appWindow;
	if (szTitle){
		tags[1].ti_Tag = ASLFR_TitleText;
		tags[1].ti_Data = (ULONG)szTitle;
	}
	if (szDrawer){
		tags[2].ti_Tag = ASLFR_InitialDrawer;
		tags[2].ti_Data = (ULONG)szDrawer;
	}
	if (szPattern){
		tags[3].ti_Tag = ASLFR_InitialPattern;
		tags[3].ti_Data = (ULONG)szPattern;
	}
	if (AslRequest(myApp->mainWnd.fileRequester, tags)){
		return myApp->mainWnd.fileRequester;
	}
	return NULL;
}

BOOL createMenu(Wnd *wnd, struct NewMenu *newm)
{
	App *myApp = wnd->app;
	
	wnd->menu = CreateMenusA(newm, 0);
	if (!wnd->menu){
		return FALSE;
	}
	
	if (!LayoutMenusA(wnd->menu, wnd->app->visual,0)){
		FreeMenus(wnd->menu);
		wnd->menu = NULL;
		return FALSE;
	}
	
	SetMenuStrip(wnd->appWindow, wnd->menu);
	
	return TRUE;
}

void setWakeTimer(App *myApp, ULONG secs, ULONG micros)
{
	myApp->wake_secs = secs;
	myApp->wake_micros = micros;
}

int dispatch(App *myApp)
{
	struct IntuiMessage *msg;
	struct Gadget *msgGadget;
	struct MenuItem *mi;
	struct MenuSelect *ms;
	Wnd *thisWnd = NULL ;
	AppGadget *g = NULL;
	ULONG app_sig = 0, sigs = 0;
	BOOL intuiTick = FALSE ;
	
	myApp->closedispatch = FALSE ;
	
	app_sig = 1 << myApp->msgPort->mp_SigBit;
    
	while (myApp->closedispatch == FALSE) {
		if (myApp->wake_secs > 0 || myApp->wake_micros > 0){
			sigs = _waitTimer(myApp->tmr, myApp->wake_secs,myApp->wake_micros, myApp->wake_sigs | app_sig);
		}else{
			sigs = Wait(myApp->wake_sigs | app_sig);
		}

		if (sigs & app_sig){
			while ((msg = GT_GetIMsg(myApp->msgPort))){
				thisWnd = (Wnd*)msg->IDCMPWindow->UserData;
				GT_ReplyIMsg(msg);
				if (myApp->modalWnd == NULL || thisWnd == myApp->modalWnd){
					switch(msg->Class){
						case IDCMP_CLOSEWINDOW:
							if (thisWnd == &myApp->mainWnd){
								// Close main app and message queue
								myApp->closedispatch = TRUE ;
							}else{
								//GT_ReplyIMsg(msg);
								closeAppWindow(thisWnd);
								//continue;
							}
							break;
						case IDCMP_INTUITICKS:
							intuiTick = TRUE ;
							if (thisWnd->fn_intuiTicks){
								thisWnd->fn_intuiTicks(thisWnd) ;
							}
							break;
						case IDCMP_GADGETUP:
							msgGadget = (struct Gadget*)msg->IAddress;
							g = (AppGadget *)(msgGadget->UserData);
							if (g->fn_gadgetUp){
								g->fn_gadgetUp(g, msg);
							}
							break;
						case IDCMP_MENUPICK:
							if (thisWnd->menu){
								mi = ItemAddress(thisWnd->menu, msg->Code);
								if (mi){
									ms = GTMENUITEM_USERDATA(mi);
									if (ms){
										ms->fn_menuselect(thisWnd, msg->Code);
									}
								}
							}
							break;
						case IDCMP_INACTIVEWINDOW:
							if (thisWnd->modal){
								//ActivateWindow(thisWnd->appWindow);
							}
						case IDCMP_ACTIVEWINDOW:
						case IDCMP_NEWSIZE:
						case IDCMP_CHANGEWINDOW:
						case IDCMP_REFRESHWINDOW:
							if (thisWnd->fn_windowEvent){
								thisWnd->fn_windowEvent(thisWnd,msg->Class);
							}
							break;
					} 
				}
				
			}
		}
		
		if (sigs & ~app_sig || intuiTick || sigs == 0){
			// other signals raised
			if (myApp->fn_wakeSigs){
				myApp->fn_wakeSigs(myApp, sigs & myApp->wake_sigs);
			}
		}
	}
    return 0;
}
/* Example callback
void processGadgetUp(AppGadget *g, struct IntuiMessage *m)
{
    switch(g->gadgetkind){
        case LISTVIEW_KIND:
        case BUTTON_KIND:
        case INTEGER_KIND:
        case STRING_KIND:
        default:
            break;
    }

}
*/