#!/bin/bash

#GBS_ROOT=/home/jean/0507.001
#PKGFILE=`ls -t $GBS_ROOT/local/repos/slp/armv7l/RPMS/org.tizen.email-0.* | head -n 1`
PKGFILE=$1
IMGDIR=$2
RM=$3

if [ -z "$PKGFILE" -o ! -f "$PKGFILE" ]; then
	echo "$PKGFILE not exists"
	exit
fi

if [ -z "$IMGDIR" ]; then
	echo "IMGDIR is empty"
	exit
fi

if [ -z "$RM" ]; then
	RM=0
fi

WD=`pwd`
DIR=/tmp/$$_$RANDOM
mkdir $DIR
#IMGDIR=$DIR
cd $DIR
for installed_file in `rpm2cpio $PKGFILE | cpio -dmiv 2>&1 | egrep '^\./' | egrep -iv '\.(mo|png|js|css)$' | grep -iv 'signature'`
do
	strings $installed_file | grep -i '\.png'  | sed 's/\(.*\/\)\?\([^/]\+\.png\).*/\2/i' >> all
done
sort all | uniq > images
cd $WD

mkdir tmp_img # debug
for i in `find $IMGDIR -type f -name "*.png"`
do
	grep `basename $i` $DIR/images > /dev/null
	if [ $? -ne 0 ]; then
		echo $i
		cp $i tmp_img # debug
		if [ $RM -ne 0 ]; then rm $i -f; fi
	fi
done

#rm -rf $DIR
