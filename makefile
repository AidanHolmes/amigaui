#   Copyright 2024 Aidan Holmes
#   Amiga UI wrapper for GadTools
# Makefile for ui.lib and test application

#SCOPTS = DEFINE=_DEBUG IGNORE=193 debug=full IncludeDirectory=/CGraphX/C/Include/
SCOPTS = OPTIMIZE Optimizerinline OptimizerComplexity=10 OptimizerGlobal OptimizerDepth=1 OptimizerLoop OptimizerTime OptimizerSchedule OptimizerPeephole IGNORE=193 IncludeDirectory=/CGraphX/C/Include/

all: Test ui.lib ImageTest

clean:
	delete \#?.o \#?.lnk \#?.map \#?.gst ui.lib Test ImageTest

Test: ui.lib main.o
	sc link to Test main.o library=ui.lib
	
ImageTest: ui.lib imagetest.o iff.o
	sc link to ImageTest imagetest.o iff.o library=ui.lib

ui.lib: app.o listgad.o stringgad.o txtgad.o iff.o numgad.o intgad.o
	oml ui.lib app.o listgad.o stringgad.o txtgad.o iff.o numgad.o intgad.o

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

imagetest.o: imagetest.c
   sc $(SCOPTS) imagetest.c
