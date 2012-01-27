#!/bin/bash

buildfile()
{
	BUILDDATE=`date +"%d/%m/%Y"`
	BUILDTIME=`date +"%H:%M"`

	echo "#define BUILDNUM $BUILDNUM" > _build.h
	echo "#define BUILDDATE \"$BUILDDATE\"" >> _build.h
	echo "#define BUILDTIME \"$BUILDTIME\"" >> _build.h
}

echo "incbuild $@, $PWD"

if [ -e _build.h ] ; then
  BUILDNUM=`cat _build.h | head -1 | cut -c18-`
else
  BUILDNUM=0
  buildfile
fi

if [ -e .incbuild ] ; then
  if [ "$1" == "--clean" ] ; then
	echo "Removing build object..."
    \rm build.o
  else
	echo "Increasing build number..."
	
	BUILDNUM=`echo "$BUILDNUM + 1" | bc`
	buildfile
  fi
fi
