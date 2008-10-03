include ../Makefile.inc

BINARY=../bin/tUAc
OBJS=tUAc.o ../ua.o ../Conn/Conn.o ../common/EDFConn.o ../client/UAClient.o ../EDF/EDF.o ../EDF/EDFElement.o ../EDF/EDFParser.o ../useful/useful.o ../useful/StackTrace.o ../client/CliUser.o ../client/CliFolder.o

all:	$(BINARY)
clean:
	@\rm -f $(BINARY) $(OBJS)

$(BINARY):	$(OBJS)
	$(LD) -o $(BINARY) $(OBJS) $(LDFLAGS) $(LDSECURE)

tUAc.o:	tUAc.cpp ../client/UAClient.h ../common/EDFConn.h ../Conn/Conn.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDF.h ../EDF/EDFElement.h ../ua.h

include ../Makefile.dep
