#ifndef __APP_HDR
#define __APP_HDR

#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/exec.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/asl.h>
#include <libraries/asl.h>
#include "compatibility.h"

#define GFX_IS_OCS  0x0001
#define GFX_IS_ECS  0x0002
#define GFX_IS_AGA	0x0003


#define INIT_APPGADGET(name,type,x,y,w,h,txt,font,id,flags) \
	AppGadget name = {type,{x,y,w,h,(UBYTE*)#txt,font,id,flags,NULL,NULL},NULL,NULL,NULL,NULL,NULL,NULL}

struct AppGadget;
struct App;
struct Wnd;

typedef void (*callbackGadgetup)(struct AppGadget *g, struct IntuiMessage *m) ;
typedef void (*callbackIntuiTicks) (struct Wnd *myWnd);
typedef void (*callbackWakeSig) (struct App *myApp, ULONG sigs);
typedef void (*callbackWindowEvent) (struct Wnd *myWnd, ULONG idcmp);
typedef void (*callbackMenuSelect) (struct Wnd *myWnd, UWORD code);

typedef struct MenuSelect {
	callbackMenuSelect fn_menuselect;
} MenuSelect;

typedef struct Wnd {
	struct Node _n;
	struct App *app ;
	struct Wnd *parent;
	BOOL modal ;
	ULONG idcmp ;
	struct NewWindow info;
	struct Window *appWindow;
    struct Gadget *gtail;
	struct List childWnd;
	struct Wnd *lastModal;
	struct FileRequester *fileRequester;
	struct Menu *menu;
	callbackIntuiTicks fn_intuiTicks ; // IDCM_INTUITICKS
	callbackWindowEvent fn_windowEvent;
} Wnd;

typedef struct App {
	struct NewScreen screeninfo;
    struct Screen *appScreen; // suggest calling getAppScreen instead of this directly. It will be old info
	struct MsgPort *msgPort;
	struct DrawInfo *screenDrawInfo; // use function to get this data (will be NULL if accessed directly)
    APTR visual;
	BOOL customscreen;
	struct Wnd mainWnd;
	struct IORequest *tmr;
	ULONG wake_secs;
	ULONG wake_micros;
	ULONG wake_sigs;
	callbackWakeSig fn_wakeSigs;
	struct Wnd *modalWnd;
	int closedispatch;
	BOOL isScreenRTG;
	struct Library *cgfx;
	struct Library *asl;
	struct Library *gfx;
	struct Library *intu;
	struct Library *gadtools;
	struct Library *util;
	void *appContext ; // anything you want
} App;

typedef struct AppGadget {
    ULONG gadgetkind ;
    struct NewGadget def;
    struct TagItem *gadgetTags;
    struct Gadget *gadget;
    Wnd *wnd;
    void *data;
	APTR oldRenderer;
    callbackGadgetup fn_gadgetUp; // IDCM_GADGETUP
	
} AppGadget;

// Returns identification of chipset in-use on target machine. 
// ECS and AGA are safe but OCS doesn't offically identify, but code attempts to retry checks to ensure correct.
UWORD gfxChipSet(void);

// V36 friendly call to determine depth. Will use V39 GetBitMapAttr if available
ULONG getScreenDepth(App *myApp);

// V36 friendly call to determine height. Will use V39 GetBitMapAttr if available
ULONG getScreenHeight(App *myApp);

// V36 friendly call to determine width. Will use V39 GetBitMapAttr if available
ULONG getScreenWidth(App *myApp);

// Must call this first before any other operations
// Returns 0 on success
int initialiseApp(App *myApp);

// Creates a screen for the App. Only works with standard hi/lo res and laced modes.
// Must be called prior to any opening of windows and after initialiseApp!
// If this fails then do not continue with application!
int createAppScreen(App *myApp, BOOL hires, BOOL laced, ULONG *tags);

// Create a clear and ready to use AppGadget. This can be used or the macro INIT_APPGADGET.
// If you don't set txtAt to any font then a default topaz font will be specified.
void initAppGadget(AppGadget *gad, ULONG type, WORD x, WORD y, WORD w, WORD h, char *txt, struct TextAttr* txtAt, UWORD id, ULONG flags);

// Just refresh this gadget
void refreshAppGadget(AppGadget *gad);

// Creates and adds a new app gadget to the window. This will reset all callbacks so only change these after this call
// Returns NULL if failed
struct Gadget* addAppGadget(Wnd *myWnd, AppGadget *gad);

// Remove a gadget from the window. Note that you will need to call addAppGadget again to reinsert.
// This doesn't delete and free the gadget
void removeAppGadget(AppGadget *gad);

// Delete and free resources for the appgadget. A new call to addAppGadget must be made to 
// recreate resources. All definitions are retained in the delete process so shouldn't need a new initAppGadget call.
// Any custom drawing routines will need reapplying after addAppGadget is called again. 
void deleteAppGadget(AppGadget *gad);

// Add a custom border render for a gadget. This manages the gadget to disable image processing
// and ensures clean deletion of gadget when app closes.
// Allocated Border objects will need to be managed by the caller and resources freed.
BOOL setCustomBorder(AppGadget *gad, struct Border *b);

// Set a tag list for the gadget. This only sets the initial gadget list before calling addAppGadget.
// This function will change the tags before or after calling openAppWindow.
void setGadgetTags(AppGadget *g, ULONG *tags);

// Process the opening of the window. Gadgets must be setup prior to this function.
// Returns 0 on success
int openAppWindow(Wnd *myWnd, ULONG *tags);

// Close an open Wnd object. It will not destroy the Wnd object but will 
// remove memory for the actual window and close it.
void closeAppWindow(Wnd *myWnd);

// Starts the application wait loop and processing of gadget and intuition messages. 
// Callbacks will be made to functions defined in the AppGadget that need to be set to process anything.
// This is a blocking function. App clean up should happen after this function completes.
// Returns 0 on success
int dispatch(App *myApp);

// Clean up call to use when destroying a window and all gadgets
void appCleanUp(App *myApp);

// No need to directly call - maybe make this private!
int initialiseWnd(App *myApp, Wnd *myWnd, UBYTE *wndTitle);

// No need to directly call - maybe make this private!
void wndCleanUp(Wnd *myWnd);

// Provide pointer to main window from application
struct Wnd* getAppWnd(App *app);

// Set size, can be called before or after openAppWindow
void wndSetSize(Wnd *myWnd, WORD width, WORD height);

// Change a window IDCMP list. Can be called before or after openAppWindow.
void wndSetIDCMP(Wnd *myWnd, ULONG idcmp);

// Will search for a Wnd object by name.
Wnd* findAppWindowName(App *myApp, UBYTE *strfind);

// Will search for a Wnd object from a native struct Window pointer. 
// Exhaustive search of windows in app. 
Wnd* findAppWindow(App *myApp, struct Window *find);

// Create a new child window. This returns the Wnd object
// to set further gadgets and IDCMP. title can be NULL.
Wnd* addChildWnd(Wnd *wnd, UBYTE *name, UBYTE *title);

// Set secondary window relative to primary. If centre is true then
// window will be central to primary (if possible), otherwise it will align
// to top left of primary.
// It's not required that either windows are open as the cached info attributes
// will be used.
void wndSetRelativePos(Wnd *primary, Wnd *secondary, BOOL centre);

// Set window as modal (if it has a parent window)
void wndSetModal(Wnd *myWnd, BOOL modal);

// Facility for ASL file requester. Parent must be set, but other parameters can be NULL.
// Returns file requester object if accepted or NULL if cancelled.
struct FileRequester* openFileLoad(Wnd *parent, UBYTE *szTitle, UBYTE *szDrawer, UBYTE *szPattern);

// Add a menu
BOOL createMenu(Wnd *wnd, struct NewMenu *newm);

// Setup a wake timer which calls fn_wakeSigs at the timeout set by
// calling this funciton. 
// Set secs and millis to zero to turn off the periodic waking.
// fn_wakeSigs will be called with a zero value in sigs when called
// by a timeout.
void setWakeTimer(App *myApp, ULONG secs, ULONG micros);

// Obtains a drawinfo struct regarding the current screen.
// You do not need to free this struct, it will be managed by the App
struct DrawInfo* getScreenDrawInfo(App *myApp);

// Safer way to get the screen. Will lock and obtain the latest screen info and then unlock again.
struct Screen* getAppScreen(App *myApp);

#endif