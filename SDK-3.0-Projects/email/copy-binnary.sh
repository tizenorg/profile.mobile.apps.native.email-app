DIR_NAME=$1
BINNARY_NAME=$2

strip $BINNARY_NAME --strip-debug --strip-unneeded --preserve-dates

mkdir -p ../../email/$DIR_NAME
cp -f $BINNARY_NAME ../../email/$DIR_NAME/$BINNARY_NAME