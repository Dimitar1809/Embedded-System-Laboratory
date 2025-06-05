# Makefile for MinGW and/or Cygwin

SOURCES_DLL=PositionControllerTilt.c EulerAngles.c MotionProfiles.c xxfuncs.c xxinverse.c xxmatrix.c xxmodel.c

ifeq ($(MAKECMDGOALS),debug)
  CXXFLAGS=-g -D_DEBUG
  DEBUG=1
else
  CXXFLAGS=-O2
  DEBUG=0
endif

debug: dll
	

default: dll

clean:
	rm PositionControllerTilt.dll
	rm -rf Debug* Release*

dll:
	$(CXX) $(CXXFLAGS) -o PositionControllerTilt.dll -shared $(SOURCES_DLL)
	@[ $(DEBUG)==0 ] && strip PositionControllerTilt.dll

