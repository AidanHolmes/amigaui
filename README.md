# AmigaUI library
This library is a C wrapper around the Amiga GadTools API. It should be compatible enough to run from version 2.0 OS/ROMs (V36) up to latest Amiga OS 3.2.
Not too much has been changed from the standard GadTools way of working, but most complexities have been hidden behind a simplier add/remove, get/set approach.
The biggest quality of life is the dispatch routine which hides all the message processing. Objects just need to set callback functions and the dispatch routine will call these instead of needing to directly interact with the messages received.
Another quality of life thing is the ability to add more signals to the dispatch processing, including a periodic callback timer (works even if not the focus window).

Not everything is in this. Not everything has been simplified enough. For instance; menus still require a NewMenu structure building and doesn't handle changes to the menu mid-application.

RTG is a consideration for the UI library. If CyberGraphics.library is installed then it will be checked and used for RTG screens. IFF and GFX modules will check for RTG and run routines according to the display.
This is more for convenience than performance and stops problems where the blitter or sprites are assumed to be available.

## App
This is the main wrapper around the UI. A default window is associated and the dispatch queue runs from this concept.
Wnd objects are included in this file. Wnd objects are linked to the main Wnd which is linked to the App. Other windows are called 'child' windows and are named so they can be opened from anywhere in the code.

## IFF
This will open an IFF and return all chunks in a callback. The callback will give a stacked view of the chunks to know where you are in the structure. 
For ease there is a parseIFFImage wrapper which handles IFF images, and should be good enough for most IFF encapsulated files. 
An extensive example of the IFF parser can be seen in the [Magazine](https://github.com/AidanHolmes/magazine) application, which is used for disk mag releases. 

## Gad files
Include these files in your main application to pull in helper functions to handle individual gadgets.

IntGad, NumGad, StringGad, TxtGad are all text entry or viewing controls. Buttons are in the App file.

## Building
All files are written for SAS/C and VBCC compilers. 
The root directory has a standard makefile which generates the required debug and release makefiles in the directories
```
/amigaui/Release
/amigaui/Debug
```

To build with Cybergraphics support (RTG) the [CyberGraphX Developer Kit Release VI](https://aminet.net/package/dev/misc/CGraphX-DevKit) should be installed into a sibling folder /lib.
After extracting you should have 
```
/lib/CGraphX
/amigaui
```

You can install elsewhere but you should update the makefile to indicate the location. 

Building Debug and Release targets gives you a ui.lib static library, which is then linked by other applications.
There are a couple of test applications provided to verify the build. **Test** should open a new screen and display a low colour UI. TestImage will load an IFF file and display on new screen (will need CTRL-C to close from shell).

### SAS/C 6.5
Simple usage from root directory to build everything:
```
smake
```

Detailed builds are:
```
smake all
smake clean
smake Release/makefile
smake Debug/makefile
```

The **all** build will invoke the build for Debug and Release targets.
The **makefile** builds will only create the makefiles in Release or Debug target directories and not invoke a build

### VBCC
You will need the standard Linux make build tool. 
Simple usage to build everything:
> make -f makefile.vbcc

Detailed builds are:
```
make -f makefile.vbcc all
make -f makefile.vbcc clean
make -f makefile.vbcc Release/makefile
make -f makefile.vbcc Debug/makefile
```
The **all** build will invoke the build for Debug and Release targets.
The **makefile** builds will only create the makefiles in Release or Debug target directories and not invoke a build