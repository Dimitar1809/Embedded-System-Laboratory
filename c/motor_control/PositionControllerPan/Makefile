# Makefile for MinGW and/or Cygwin

SOURCES_DLL=PositionControllerPan.c EulerAngles.c MotionProfiles.c xxfuncs.c xxinverse.c xxmatrix.c xxmodel.c

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
	rm PositionControllerPan.dll
	rm -rf Debug* Release*

dll:
	$(CXX) $(CXXFLAGS) -o PositionControllerPan.dll -shared $(SOURCES_DLL)
	@[ $(DEBUG)==0 ] && strip PositionControllerPan.dll

