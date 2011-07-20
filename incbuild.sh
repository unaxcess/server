#!/bin/sh

buildfile()
{
	BUILDDATE=`date +"%d/%m/%Y"`
	BUILDTIME=`date +"%H:%M"`

	echo "#define BUILDNUM $BUILDNUM" > build.h
	echo "#define BUILDDATE \"$BUILDDATE\"" >> build.h
	echo "#define BUILDTIME \"$BUILDTIME\"" >> build.h
}

echo "incbuild $PWD"

test -e build.h
# echo "incbuild build.h: $?"
if [ $? = 0 ] ; then
  BUILDNUM=`cat build.h | head -1 | cut -c18-`
else
  BUILDNUM=0
  buildfile
fi

test -e .incbuild
# echo "incbuild .incbuild: $?"
if [ $? = 0 ] ; then
	echo "Increasing build number..."
	
	BUILDNUM=`echo "$BUILDNUM + 1" | bc`
	buildfile
fi

