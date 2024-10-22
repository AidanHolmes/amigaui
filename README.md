# AmigaUI library
This library is a C wrapper around the Amiga GadTools API. It should be compatible enough to run from version 2.0 OS/ROMs up to latest Amiga OS 3.2.
Not too much has been changed from the standard GadTools way of working, but most complexities have been hidden behind a simplier add/remove, get/set approach.
The biggest quality of life is the dispatch routine which hides all the message processing. Objects just need to set callback functions and the dispatch routine will call these instead of needing to directly interact with the messages received.
Another quality of life thing is the ability to add more signals to the dispatch processing, including a periodic callback timer (works even if not the focus window).

Not everything is in this. Not everything has been simplified enough. For instance; menus still require a NewMenu structure building and doesn't handle changes to the menu mid-application.

## App
This is the main wrapper around the UI. A default window is associated and the dispatch queue runs from this concept.
Wnd objects are included in this file. Wnd objects are linked to the main Wnd which is linked to the App. Other windows are called 'child' windows and are named so they can be opened from anywhere in the code.

## IFF
Work in progress library. This will open an IFF and return all chunks in a callback. The callback will give a stacked view of the chunks to know where you are in the structure. 
For ease there is a parseIFFImage wrapper which handles IFF images, and should be good enough for most files.

## Gad files
Include these files in your main application to pull in helper functions to handle individual gadgets.

IntGad, NumGad, StringGad, TxtGad are all text entry or viewing controls. Buttons are in the App file.

## Building
All files are written for SAS/C. This *should* build in any compiler, but some specific macros and auto library creation may require some changes.
The makefile is written for smake. 
Run smake to build all files.
