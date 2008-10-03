include ../Makefile.inc

OBJS=../ua.o ../Conn/Conn.o ../common/EDFConn.o ../client/UAClient.o ../EDF/EDF.o ../EDF/EDFElement.o ../EDF/EDFParser.o ../useful/useful.o ../useful/StackTrace.o ../client/CliUser.o ../client/CliFolder.o

QUALMBINARY=../bin/qUAlm
QUALMOBJS=qUAlm.o ../client/UAService.o ../Conn/SMTPConn.o

GUARDEDBINARY=../bin/gUArded
GUARDEDOBJS=gUArded.o ../client/UAService.o ../Conn/SMTPConn.o

HOGWUARTSBINARY=../bin/hogwUArts
HOGWUARTSOBJS=hogwUArts.o

LOCATORBINARY=../bin/Locator
LOCATOROBJS=Locator.o

ALLBINARY=$(SPELLUABINARY) $(QUALMBINARY) $(GUARDEDBINARY) $(HOGWUARTSBINARY)
ALLOBJS=$(OBJS) $(SPELLUAOBJS) $(QUALMOBJS) $(GUARDEDOBJS) $(HOGWUARTSOBJS)

HEADERS=../client/UAClient.h ../common/EDFConn.h ../Conn/Conn.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDF.h ../EDF/EDFElement.h ../EDF/EDFParser.h ../ua.h

all:  $(GUARDEDBINARY) $(HOGWUARTSBINARY)
clean:
	@\rm -f $(ALLBINARY) $(ALLOBJS)

$(SPELLUABINARY): $(SPELLUAOBJS) $(OBJS)
	$(LD) -o $@ $(SPELLUAOBJS) $(OBJS) $(LDFLAGS) $(LDSECURE)

$(QUALMBINARY): $(QUALMOBJS) $(OBJS)
	$(LD) -o $@ $(QUALMOBJS) $(OBJS) $(LDFLAGS) $(LDSECURE)

$(GUARDEDBINARY): $(GUARDEDOBJS) $(OBJS)
	$(LD) -o $@ $(GUARDEDOBJS) $(OBJS) $(LDFLAGS) $(LDSECURE)
	
$(HOGWUARTSBINARY):	$(HOGWUARTSOBJS) $(OBJS)
	$(LD) -o $@ $(HOGWUARTSOBJS) $(OBJS) $(LDFLAGS) $(LDSECURE)

$(LOCATORBINARY):	$(LOCATOROBJS) $(OBJS)
	$(LD) -o $@ $(LOCATOROBJS) $(OBJS) $(LDFLAGS) $(LDSECURE)

qUAlm.o:	qUAlm.cpp ../Conn/SMTPConn.h qUAlm.h $(HEADERS)

gUArded.o:	gUArded.cpp ../Conn/SMTPConn.h gUArded.h $(HEADERS)

hogwUArts.o:hogwUArts.cpp $(HEADERS)

Locator.o:Locator.cpp $(HEADERS)

include ../Makefile.dep
