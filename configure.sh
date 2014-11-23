#!/bin/sh

makeCheck()
{
# echo "Testing $1/Makefile"
test -e $1/Makefile
if [ $? = 0 ] ; then
	echo -e "\t\tcd $1 ; make $2"
#else
#	echo "Does not exist"
fi
}

doMakefile()
{
echo "# UNaXcess II Conferencing System"
echo "# (c) 1999 Michael Wood (mike@compsoc.man.ac.uk)"
echo "#"
echo "# Build file for sub directories (delete as appropriate)"
echo ""

DIRS=`find * -type d -maxdepth 0 -follow`

echo "all:"
for DIR in $DIRS
do
	if [ "$DIR" = "qUAck" ] ; then
		# echo Checking qUAck
		makeCheck qUAck ../bin/qUAck
	else
		makeCheck $DIR
	fi
done

echo ""

echo "clean:"
for DIR in $DIRS
do
	makeCheck $DIR clean
done

echo ""

echo "install:"
for DIR in $DIRS
do
	makeCheck $DIR install
done
}

doSuffix()
{
echo ".$1.o:"
echo -e "\t\t\$(CC) \$(CCFLAGS) -c $< -o \$(@)"
echo ""
}

doMakefileInc()
{
echo "# UNaXcess II Conferencing System"
echo "# (c) 1999 Michael Wood (mike@compsoc.man.ac.uk)"
echo "#"
echo "# Makefile.inc: Common build options"
echo "#"
echo "# - To reduce warning output remove -Wall"
echo "# - To turn of SSL remove CCSECURE and LDSECURE"
echo ""

echo "SSLINC=/usr/local/ssl/include"
echo "SSLLIB=/usr/local/ssl/lib"
echo ""

arch=`uname`
echo "CC=g++"
echo "CCSECURE=-DCONNSECURE -I\$(SSLINC)"
echo "INCCCFLAGS=-g -Wall -O2 -DUNIX -D$arch -DSTACKTRACEON -I.."
echo "CCFLAGS=\$(INCCCFLAGS) \$(CCSECURE)"
echo ""

echo "LD=g++"
echo "LDSECURE=-L\$(SSLLIB) -lssl -lcrypto"
echo "LDFLAGS=\$(LDSECURE)"
echo ""

echo ".SUFFIXES:\$(SUFFIXES) .cpp .cc .c"
echo ""
doSuffix cpp
doSuffix cc
doSuffix c
}

doMakefileDep()
{
test -d /usr/local/mysql
if [ $? = 0 ] ; then
	echo "DBINC=/usr/local/mysql/include"
	echo "DBLIB=/usr/local/mysql/lib"
else
	echo "DBINC=/usr/include/mysql"
	echo "DBLIB=/usr/lib/mysql"
fi
}

test -e Makefile
if [ $? != 0 ] ; then
	echo "Creating Makefile..."
	doMakefile > Makefile
fi

test -e Makefile.inc
if [ $? != 0 ] ; then
	echo "Creating include Makefile..."
	doMakefileInc >> Makefile.inc
fi

test -e server
if [ $? = 0 ] ; then
	test -e server/Makefile.db
	if [ $? != 0 ] ; then
		echo "Creating database Makefile..."
		doMakefileDep > server/Makefile.db
	fi
fi

test -d bin
if [ $? != 0 ] ; then
	mkdir bin
fi

test -e server
if [ $? = 0 ] ; then
	test -e bin/uadata.edf
	if [ $? != 0 ] ; then
		echo "Creating initial data file..."
		
		touch bin/uadata.edf
		echo "<>" >> bin/uadata.edf
		echo "  <system>" >> bin/uadata.edf
		echo "    <database=\"ua\"/>" >> bin/uadata.edf
		echo "  </system>" >> bin/uadata.edf
		echo "</>" >> bin/uadata.edf
	fi
fi

for FILE in Makefile* */Makefile *.[ch]* */*.[ch]*
do
	echo "Converting $FILE"
	tr -d \\r <$FILE >temp
	\mv temp $FILE
done
