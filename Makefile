# UNaXcess II Conferencing System
# (c) 1999 Michael Wood (mike@compsoc.man.ac.uk)
#
# Build file for sub directories (delete as appropriate)

all:
		cd ICE ; make 
		cd server ; make 
		cd client ; make 
		cd utils ; make 

clean:
		cd ICE ; make clean
		cd server ; make clean
		cd client ; make clean
		cd utils ; make clean
