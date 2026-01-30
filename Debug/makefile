#   Copyright 2026 Aidan Holmes
#   Amiga UI wrapper for GadTools
# 	Makefile for ui.lib and test application. This uses SAS/C smake to build
SRC = /Src
BIN = Bin
OBJ = Objs
LIBNAME = ui.lib

OBJS= $(OBJ)/app.o $(OBJ)/listgad.o $(OBJ)/stringgad.o $(OBJ)/txtgad.o $(OBJ)/iff.o $(OBJ)/numgad.o $(OBJ)/intgad.o $(OBJ)/gfx.o
OBJSTest = $(OBJ)/main.o
OBJSImageTest = $(OBJ)/imagetest.o

# Enable for non-debug and optimised code
SCOPTS = nocheckabort DEFINE=_DEBUG IGNORE=193 debug=full IncludeDirectory=//CGraphX/C/Include/ IncludeDirectory=//NDK3.2R4/Include_H
all: dirs $(BIN)/$(LIBNAME) $(BIN)/Test $(BIN)/ImageTest

dirs:
	- makedir $(BIN)
	- makedir $(OBJ)

clean:
	- delete $(OBJ)/\#?.o $(OBJ)/\#?.asm $(BIN)/\#?.lnk $(BIN)/\#?.info $(BIN)/\#?.map $(BIN)/\#?.gst $(BIN)/$(LIBNAME) $(BIN)/Test $(BIN)/ImageTest

$(BIN)/Test: $(BIN)/$(LIBNAME) $(OBJSTest)
	sc link to $(BIN)/Test $(OBJSTest) library=$(BIN)/$(LIBNAME)
	
$(BIN)/ImageTest: $(BIN)/$(LIBNAME) $(OBJSImageTest)
	sc link to $(BIN)/ImageTest $(OBJSImageTest) library=$(BIN)/$(LIBNAME)

$(BIN)/$(LIBNAME): $(OBJS)
	oml $(BIN)/$(LIBNAME) $(OBJS)
	
.c.o:
	sc $(SCOPTS) $? ObjectName=$(OBJ)/

$(OBJ)/main.o: $(SRC)/main.c 
$(OBJ)/app.o: $(SRC)/app.c 
$(OBJ)/listgad.o: $(SRC)/listgad.c 
$(OBJ)/stringgad.o: $(SRC)/stringgad.c 
$(OBJ)/txtgad.o: $(SRC)/txtgad.c 
$(OBJ)/numgad.o: $(SRC)/numgad.c 
$(OBJ)/intgad.o: $(SRC)/intgad.c 
$(OBJ)/iff.o: $(SRC)/iff.c
$(OBJ)/gfx.o: $(SRC)/gfx.c
$(OBJ)/imagetest.o: $(SRC)/imagetest.c
