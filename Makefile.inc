# UNaXcess II Conferencing System
# (c) 1999 Michael Wood (mike@compsoc.man.ac.uk)
#
# Makefile.inc: Common build options
#
# - To reduce warning output remove -Wall
# - To turn of SSL remove CCSECURE and LDSECURE

SSLINC=/usr/local/ssl/include
SSLLIB=/usr/local/ssl/lib

CC=g++
CCSECURE=-DCONNSECURE -I$(SSLINC)
INCCCFLAGS=-g -w -Wfatal-errors -O2 -DUNIX -DLinux -DSTACKTRACEON -I..
CCFLAGS=$(INCCCFLAGS) $(CCSECURE) -Wno-write-strings

LD=g++
LDSECURE=-L$(SSLLIB) -lssl -lcrypto
LDFLAGS=$(LDSECURE)

.SUFFIXES:$(SUFFIXES) .cpp .cc .c

.cpp.o:
		$(CC) $(CCFLAGS) -c $< -o $(@)

.cc.o:
		$(CC) $(CCFLAGS) -c $< -o $(@)

.c.o:
		$(CC) $(CCFLAGS) -c $< -o $(@)

