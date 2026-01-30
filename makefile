# Version - these values are mastered here and overwrite the generated values in makefiles for Debug and Release
LIBDEVMAJOR = 1
LIBDEVMINOR = 0
LIBDEVDATE = '11.01.2026'

RELEASEDIR = Release
DEBUGDIR = Debug
RELEASE = $(RELEASEDIR)/makefile
DEBUG = $(DEBUGDIR)/makefile

# This build requires an install of the CyberGraphics library (such as P96) and the dev headers. 
# Change to point to location of headers.
INCDEPENDS=IncludeDirectory=//CGraphX/C/Include/ IncludeDirectory=//NDK3.2R4/Include_H

# optimised and release version
#optdepth
# defines the maximum depth of function calls to be Mined. The
# range is 0 to 6, and the default value is 3.
PRODCOPTS = nocheckabort OPTIMIZE Optimizerinline OptimizerComplexity=30 OptimizerGlobal OptimizerDepth=6 OptimizerLoop OptimizerTime OptimizerSchedule OptimizerPeephole IGNORE=193 $(INCDEPENDS)

# debug version build options
DBGCOPTS = nocheckabort DEFINE=_DEBUG IGNORE=193 debug=full $(INCDEPENDS)

all: $(RELEASE) $(DEBUG)
	execute <<
		cd $(RELEASEDIR)
		smake LIBDEVMAJOR=$(LIBDEVMAJOR) LIBDEVMINOR=$(LIBDEVMINOR) LIBDEVDATE=$(LIBDEVDATE)
		cd /
		<
	execute <<
		cd $(DEBUGDIR)
		smake LIBDEVMAJOR=$(LIBDEVMAJOR) LIBDEVMINOR=$(LIBDEVMINOR) LIBDEVDATE=$(LIBDEVDATE)
		cd /
		<
	
clean:
	execute <<
		cd $(RELEASEDIR)
		smake clean
		cd /
		<
	execute <<
		cd $(DEBUGDIR)
		smake clean
		cd /
		<
	
$(RELEASE): makefile.master makefile
	copy makefile.master $(RELEASE)
	splat -o "^SCOPTS.+\$" "SCOPTS = $(PRODCOPTS)" $(RELEASE)
	
$(DEBUG): makefile.master makefile
	copy makefile.master $(DEBUG)
	splat -o "^SCOPTS.+\$" "SCOPTS = $(DBGCOPTS)" $(DEBUG)
