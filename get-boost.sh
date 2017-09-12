#!/bin/sh
export BOOSTVER=65
export BOOSTMINOR=1
export BDOT="1.$BOOSTVER.$BOOSTMINOR"
export BUC="boost_1_${BOOSTVER}_$BOOSTMINOR"

if [ ! -d include ]; then
	echo "This should be run from the project root folder!"
	exit
fi

rm -rfv boost*
if test -x "/usr/bin/wget"; then
	wget http://sourceforge.net/projects/boost/files/boost/$BDOT/$BUC.tar.bz2/download -O $BUC.tar.bz2
elif test -x "/usr/bin/curl"; then
	curl -L --max-redirs 10 http://sourceforge.net/projects/boost/files/boost/$BDOT/$BUC.tar.bz2/download > $BUC.tar.bz2
fi

if [ ! -f $BUC.tar.bz2 ]; then
	echo "Failed to fetch $BUC.tar.bz2!"
	exit
fi

tar -jxvf $BUC.tar.bz2 $BUC/boost ./$BUC/boost
rm -rfv include/boost
mv -vf $BUC/boost include/
rm -rfv boost*
