# UNaXcess II Conferencing System
# (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
#
# Build file for ICE executable

include ../Makefile.inc

BINARY=../bin/ICE
OBJS=../ICE/ICE.o ../useful/useful.o ../useful/StackTrace.o ../EDF/EDFElement.o ../EDF/EDF.o ../EDF/EDFParser.o ../Conn/Conn.o ../common/EDFConn.o


all:	$(BINARY)

clean:
	@\rm -f $(BINARY) $(OBJS)

$(BINARY):	$(OBJS)
	$(LD) -o $(BINARY) $(OBJS) $(LDFLAGS) -rdynamic -ldl $(LDSECURE)

install:	$(OBJS)
	\rm -f ../bin/$(BINARY)
	$(LD) -o ../bin/$(BINARY) $(OBJS) $(LDFLAGS) -rdynamic -ldl $(LDSECURE)

ICE.o:	ICE.cpp ../common/EDFConn.h ../Conn/Conn.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDF.h ../EDF/EDFElement.h ../EDF/EDFParser.h ICE.h

include ../Makefile.dep
