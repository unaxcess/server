UNaXcess II - Client / Server Conference System

Links
-----

Latest version -      http://ua2.org/UADN/ua.server.tar.gz
Buck stops here -     mike@compsoc.man.ac.uk (Techno on UA)

FAQ
---

Q: How do I build UA?
A: Type "./configure.sh" then "make" from the command line

Q: Will it work on <x>?
A: UA will compile and has been tested under

   - Linux (should work as is)
	- Windows (project files for your favourite IDE not supplied)
	
	Other systems cannot be guaranteed, but don't let that stop you.
	
Q: Make stops without compiling anything
A: It has problems with DOS style carriage return / line feeds. Run Makefile*
   */Makefile through dos2unix / duconv or load them into vi and do
	1,$s/<Ctrl>V<Ctrl>M//g

Q: "Cannot find openssl/openssl.h"
A: You don't not have SSL installed on your machine, or it is in a different
   place. Makefile.inc has SSL related stuff in it. Comment out the CCSECURE
	and LDSECURE lines

Q: "Stray '\' on line..."
A: See 'Make stops' above
   
Credits
-------

qUAck looks the way it does as a result of 10 years UI "refinement" beginning
with old UNaXcess (circa 1984) and continuing until 1999 when we [the Manc CS
UA bods] were dragged kicking and screaming onto UNaXcess II

Old UA guilty parties: Brandon S Allbery, Andrew G Minter, Rob Partington,
Gryn Davies, Michael Wood, Andrew Armitage, Francis Cook, Brian Widdas

Enjoy!

        Mike
