
include ../Makefile.def

CFLAGS	= -O2 -fPIC -g -Wall -I$(GADEPS)/include $(EXTRACFLAGS) -DPIPELINE_FILTER \
	  $(AVCCF) $(L5CF) $(SDLCF)
#	-D__STDC_CONSTANT_MACROS

ifeq ($(OS),'Linux')
CFLAGS	+= $(ASNDCF) $(X11CF)
endif

TARGET	= libga.a

all: $(TARGET)

.cpp.o:
	$(CXX) -c -g $(CFLAGS) $<

libga.a: ga-common.o ga-conf.o ga-confvar.o  ga-module.o ga-avcodec.o \
	rtspconf.o pipeline.o \
	vsource.o asource.o encoder-common.o controller.o server.o rtspserver.o
	ar rc $@ $^

install:
	# do nothing

clean:
	rm -f $(TARGET) *.o *~

