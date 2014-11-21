UNaXcess II - Client / Server Conference System
===============================================

*How do I build UA?*
Type "./configure.sh" then "make" from the command line

*Will it work on <x>?*
UA will compile and has been tested under

* Linux (should work as is)
* Windows (project files for your favourite IDE not supplied)

Other systems cannot be guaranteed, but don't let that stop you.
	
*Make stops without compiling anything*
It has problems with DOS style carriage return / line feeds. Run Makefile* */Makefile through dos2unix / duconv or load them into vi and do
1,$s/<Ctrl>V<Ctrl>M//g

*"Cannot find openssl/openssl.h"*
You don't not have SSL installed on your machine, or it is in a different place. Makefile.inc has SSL related stuff in it. Comment out the CCSECURE and LDSECURE lines

*"Stray '\' on line..."*
See 'Make stops' above
   
Credits
=======

qUAck looks the way it does as a result of 10 years UI "refinement" beginning
with old UNaXcess (circa 1984) and continuing until 1999 when we [the Manc CS
UA bods] were dragged kicking and screaming onto UNaXcess II

Old UA guilty parties: Brandon S Allbery, Andrew G Minter, Phaedrus, Gryn, Techno, Laughter, Fran, BW.

Enjoy!
