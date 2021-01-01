# UNaXcess II Conferencing System
# (c) 1999 Michael Wood (mike@compsoc.man.ac.uk)
#
# Build file for sub directories (delete as appropriate)

all:
		cd ICE ; make 
		cd server ; make 

clean:
		cd ICE ; make clean
		cd server ; make clean

install:
		cd server ; make install

container:
		docker build --file Dockerfile.build . -t unaxcess/server:latest 
