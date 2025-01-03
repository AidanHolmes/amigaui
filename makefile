#   Copyright 2025 Aidan Holmes
#   Amiga UI wrapper for GadTools
# Makefile for ui.lib and test application. This uses SAS/C smake to build

# This build requires an install of the CyberGraphics library (such as P96) and the dev headers. 
# Change to point to location of headers.
INCDEPENDS=IncludeDirectory=/lib/CGraphX/C/Include/

# Enable for debug
#SCOPTS = DEFINE=_DEBUG IGNORE=193 debug=full $(INCDEPENDS)

# Enable for non-debug and optimised code
SCOPTS = OPTIMIZE Optimizerinline OptimizerComplexity=10 OptimizerGlobal OptimizerDepth=1 OptimizerLoop OptimizerTime OptimizerSchedule OptimizerPeephole IGNORE=193 $(INCDEPENDS)

all: Test ui.lib ImageTest

clean:
	delete \#?.o \#?.lnk \#?.map \#?.gst ui.lib Test ImageTest

Test: ui.lib main.o
	sc link to Test main.o library=ui.lib
	
ImageTest: ui.lib imagetest.o iff.o
	sc link to ImageTest imagetest.o iff.o library=ui.lib

ui.lib: app.o listgad.o stringgad.o txtgad.o iff.o numgad.o intgad.o gfx.o
	oml ui.lib app.o listgad.o stringgad.o txtgad.o iff.o numgad.o intgad.o gfx.o

main.o: main.c 
   sc $(SCOPTS) main.c
   
app.o: app.c 
   sc $(SCOPTS) app.c
   
listgad.o: listgad.c 
   sc $(SCOPTS) listgad.c
   
stringgad.o: stringgad.c 
   sc $(SCOPTS) stringgad.c
   
txtgad.o: txtgad.c 
   sc $(SCOPTS) txtgad.c
   
numgad.o: numgad.c 
   sc $(SCOPTS) numgad.c
   
intgad.o: intgad.c 
   sc $(SCOPTS) intgad.c
   
iff.o: iff.c
   sc $(SCOPTS) iff.c

gfx.o: gfx.c
   sc $(SCOPTS) gfx.c

imagetest.o: imagetest.c
   sc $(SCOPTS) imagetest.c
