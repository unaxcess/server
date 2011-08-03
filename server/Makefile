# UNaXcess II Conferencing System
# (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
#
# Build file for server library

include ../Makefile.inc

include Makefile.db

CCFLAGS+=-fPIC -DDBMYSQL -I$(DBINC)

LDFLAGS+=-L$(DBLIB) -lcrypt -lz -lmysqlclient

# Server library
BINARY1=../bin/libua
BINARY2=../bin/ua
BINARY3=../bin/MessageSpec
BINARY4=../bin/Posting
BINARY5=../bin/BigThreads
BINARY6=../bin/Location
BINARY=$(BINARY2) $(BINARY1) $(BINARY3)
# OBJS=server.o DBSub.o DBMessageRead.o RqTree.o RqFolder.o RqTalk.o RqSystem.o RqUser.o ../ua.o Folder.o Tree.o Talk.o User.o ChannelMessageItem.o FolderMessageItem.o MessageTreeItem.o MessageItem.o UserItem.o EDFItem.o EDFItemList.o ../db/db-MySQL.o ../db/DBTable.o ../EDF/EDF.o ../EDF/EDFElement.o ../EDF/EDFParser.o ../useful/StackTrace.o ../Conn/Conn.o ../common/EDFConn.o ../useful/useful.o
OBJS=server.o DBSub.o DBMessageRead.o RqTree.o RqFolder.o RqTalk.o RqSystem.o RqUser.o ../ua.o Folder.o Tree.o Talk.o User.o ChannelMessageItem.o FolderMessageItem.o MessageTreeItem.o MessageItem.o UserItem.o EDFItem.o EDFItemList.o ../db/db-MySQL.o ../db/DBTable.o
OBJS1=$(OBJS)
OBJS2=$(OBJS) ../EDF/EDF.o ../EDF/EDFElement.o ../EDF/EDFParser.o ../useful/StackTrace.o ../Conn/Conn.o ../common/EDFConn.o ../useful/useful.o ICE-leak.o ../useful/LeakTrace.o
OBJS3=MessageSpec.o ../EDF/EDF.o ../EDF/EDFElement.o ../EDF/EDFParser.o ../useful/useful.o ../useful/StackTrace.o
OBJS4=Posting.o ../db/DBTable.o ../db/db-MySQL.o ../useful/useful.o ../useful/StackTrace.o ../EDF/EDFElement.o ../EDF/EDF.o ../EDF/EDFParser.o
OBJS5=BigThreads.o ../db/DBTable.o ../db/db-MySQL.o ../useful/useful.o ../useful/StackTrace.o
OBJS6=$(OBJS) Location.o Skeleton.o ../EDF/EDF.o ../EDF/EDFElement.o ../EDF/EDFParser.o ../useful/StackTrace.o ../Conn/Conn.o ../common/EDFConn.o ../useful/useful.o


all:	$(BINARY)

clean:
	@\rm -f $(BINARY) $(OBJS1) $(OBJS2)

build.h:
	@../incbuild.sh

$(BINARY1):	$(OBJS1)
	@\rm -f $(BINARY1)
	$(LD) -o $(BINARY1) $(OBJS1) $(LDFLAGS) -shared -expall

$(BINARY2):	$(OBJS2)
	$(LD) -o $(BINARY2) $(OBJS2) $(LDFLAGS) -rdynamic -ldl

$(BINARY3):	$(OBJS3)
	$(LD) -o $(BINARY3) $(OBJS3) $(LDFLAGS)

$(BINARY4):	$(OBJS4)
	$(LD) -o $(BINARY4) $(OBJS4) $(LDFLAGS)

$(BINARY5):	$(OBJS5)
	$(LD) -o $(BINARY5) $(OBJS5) $(LDFLAGS)

$(BINARY6):     $(OBJS6)
	$(LD) -o $(BINARY6) $(OBJS6) $(LDFLAGS)

install:	$(OBJS)
	@\rm -f ../bin/$(BINARY1)
	$(LD) -o ../bin/$(BINARY1) $(OBJS) $(LDFLAGS) -shared -expall

../db/DBTable.o:      ../db/DBTable.cpp ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../db/DBTable.h ../db/db-common.h
../db/db-MySQL.o:     ../db/db-MySQL.cpp ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../db/DBTable.h ../db/db-common.h

DBItemList.o:	DBItemList.cpp
DBMessageRead.o:	DBMessageRead.cpp ../db/DBTable.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h DBMessageRead.h DBItemList.h ../server/ItemList.h ItemList.cpp DBItemList.cpp
DBSub.o:	DBSub.cpp DBSub.h DBItemList.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../server/ItemList.h ItemList.cpp DBItemList.cpp ../db/DBTable.h ../EDF/EDF.h ../EDF/EDFElement.h ../EDF/EDFParser.h ../ua.h

ItemList.o:	ItemList.cpp

EDFItem.o:	EDFItem.cpp EDFItem.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../db/DBTable.h
EDFItemList.o:	EDFItemList.cpp EDFItemList.h EDFItem.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../db/DBTable.h

ChannelMessageItem.o:	ChannelMessageItem.cpp ../ua.h ChannelMessageItem.h MessageTreeItem.h TreeNodeItem.h EDFItem.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../db/DBTable.h EDFItemList.h TreeNodeItem.cpp

MessageTreeItem.o:	MessageTreeItem.cpp ../ua.h MessageTreeItem.h TreeNodeItem.h EDFItem.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../db/DBTable.h EDFItemList.h TreeNodeItem.cpp MessageItem.h
MessageItem.o:	MessageItem.cpp ../ua.h MessageItem.h TreeNodeItem.h EDFItem.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../db/DBTable.h EDFItemList.h TreeNodeItem.cpp MessageTreeItem.h
FolderMessageItem.o:	FolderMessageItem.cpp ../ua.h FolderMessageItem.h MessageTreeItem.h TreeNodeItem.h EDFItem.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../db/DBTable.h EDFItemList.h TreeNodeItem.cpp
UserItem.o:	UserItem.cpp ../ua.h ../Conn/Conn.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h UserItem.h EDFItem.h ../EDF/EDF.h ../EDF/EDFElement.h ../EDF/EDFParser.h ../db/DBTable.h

Folder.o:	Folder.cpp EDFItemList.h EDFItem.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../db/DBTable.h Tree.h MessageTreeItem.h TreeNodeItem.h TreeNodeItem.cpp MessageItem.h Folder.h FolderMessageItem.h ../ua.h
Talk.o:	Talk.cpp EDFItemList.h EDFItem.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../db/DBTable.h Tree.h MessageTreeItem.h TreeNodeItem.h TreeNodeItem.cpp MessageItem.h Talk.h ChannelMessageItem.h ../ua.h
Tree.o:	Tree.cpp EDFItemList.h EDFItem.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../db/DBTable.h Tree.h MessageTreeItem.h TreeNodeItem.h TreeNodeItem.cpp MessageItem.h ../ua.h
User.o:	User.cpp EDFItemList.h EDFItem.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../db/DBTable.h User.h UserItem.h

RqFolder.o:	RqFolder.cpp headers.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../common/EDFConn.h ../Conn/Conn.h ../db/DBTable.h ../ua.h ../ICE/ICE.h ../server/Folder.h MessageTreeItem.h TreeNodeItem.h EDFItem.h EDFItemList.h TreeNodeItem.cpp MessageItem.h FolderMessageItem.h ../server/User.h UserItem.h server.h ../server/MessageItem.h DBSub.h DBItemList.h ../server/ItemList.h ItemList.cpp DBItemList.cpp DBMessageRead.h RqTree.h ../server/MessageTreeItem.h ../server/FolderMessageItem.h ../server/UserItem.h ../server/DBSub.h RqFolder.h ../server/DBMessageRead.h RqUser.h
RqSystem.o:	RqSystem.cpp headers.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../common/EDFConn.h ../Conn/Conn.h ../db/DBTable.h ../ua.h ../ICE/ICE.h ../server/Folder.h MessageTreeItem.h TreeNodeItem.h EDFItem.h EDFItemList.h TreeNodeItem.cpp MessageItem.h FolderMessageItem.h ../server/Talk.h ChannelMessageItem.h server.h ../server/User.h UserItem.h ../server/MessageItem.h DBSub.h DBItemList.h ../server/ItemList.h ItemList.cpp DBItemList.cpp DBMessageRead.h RqFolder.h ../server/DBMessageRead.h RqUser.h ../server/UserItem.h RqSystem.h
RqTalk.o:	RqTalk.cpp headers.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../common/EDFConn.h ../Conn/Conn.h ../db/DBTable.h ../ua.h ../ICE/ICE.h ../server/MessageTreeItem.h TreeNodeItem.h EDFItem.h EDFItemList.h TreeNodeItem.cpp MessageItem.h ../server/Talk.h ChannelMessageItem.h server.h ../server/User.h UserItem.h ../server/MessageItem.h DBSub.h DBItemList.h ../server/ItemList.h ItemList.cpp DBItemList.cpp DBMessageRead.h RqTree.h ../server/FolderMessageItem.h ../server/UserItem.h ../server/DBSub.h RqTalk.h
RqTree.o:	RqTree.cpp headers.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../common/EDFConn.h ../Conn/Conn.h ../db/DBTable.h ../ua.h ../ICE/ICE.h ../server/Folder.h MessageTreeItem.h TreeNodeItem.h EDFItem.h EDFItemList.h TreeNodeItem.cpp MessageItem.h FolderMessageItem.h ../server/Talk.h ChannelMessageItem.h server.h ../server/User.h UserItem.h ../server/MessageItem.h DBSub.h DBItemList.h ../server/ItemList.h ItemList.cpp DBItemList.cpp DBMessageRead.h RqTree.h ../server/MessageTreeItem.h ../server/FolderMessageItem.h ../server/UserItem.h ../server/DBSub.h RqFolder.h ../server/DBMessageRead.h RqTalk.h
RqUser.o:	RqUser.cpp headers.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../common/EDFConn.h ../Conn/Conn.h ../db/DBTable.h ../ua.h ../ICE/ICE.h ../server/Folder.h MessageTreeItem.h TreeNodeItem.h EDFItem.h EDFItemList.h TreeNodeItem.cpp MessageItem.h FolderMessageItem.h server.h ../server/User.h UserItem.h ../server/MessageItem.h DBSub.h DBItemList.h ../server/ItemList.h ItemList.cpp DBItemList.cpp DBMessageRead.h RqUser.h ../server/UserItem.h RqFolder.h ../server/DBMessageRead.h RqTree.h ../server/MessageTreeItem.h ../server/FolderMessageItem.h ../server/DBSub.h RqSystem.h

server.o:	build.h server.cpp headers.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../common/EDFConn.h ../Conn/Conn.h ../db/DBTable.h ../ua.h ../ICE/ICE.h server.h ../server/User.h UserItem.h EDFItem.h ../server/MessageItem.h TreeNodeItem.h EDFItemList.h TreeNodeItem.cpp DBSub.h DBItemList.h ../server/ItemList.h ItemList.cpp DBItemList.cpp DBMessageRead.h ../server/Folder.h MessageTreeItem.h FolderMessageItem.h ../server/Talk.h ChannelMessageItem.h RqTree.h ../server/MessageTreeItem.h ../server/FolderMessageItem.h ../server/UserItem.h ../server/DBSub.h RqFolder.h ../server/DBMessageRead.h RqTalk.h RqSystem.h RqUser.h build.h

ICE-leak.o:	../ICE/ICE.cpp ../common/EDFConn.h ../Conn/Conn.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDF.h ../EDF/EDFElement.h ../EDF/EDFParser.h ../ICE/ICE.h
	$(CC) $(CCFLAGS) -DLEAKTRACEON -c $< -o $(@)

MessageSpec.o:	MessageSpec.cpp ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../ua.h

SpecConv.o:	SpecConv.cpp ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h

Posting.o:      Posting.cpp headers.h ../EDF/EDF.h ../EDF/EDFElement.h ../useful/useful.h ../useful/StackTrace.h ../useful/LeakTrace.h ../EDF/EDFParser.h ../common/EDFConn.h ../Conn/Conn.h ../db/DBTable.h ../ua.h

BigThreads.o:      BigThreads.cpp ../useful/useful.h ../useful/StackTrace.h ../db/DBTable.h

Location.o:	Location.cpp

include ../Makefile.dep
