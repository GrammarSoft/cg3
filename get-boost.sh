#!/bin/sh
cd "`dirname "$0"`"
rm -rfv boost*
if test -x "/usr/bin/wget"; then
	wget http://sourceforge.net/projects/boost/files/boost/1.50.0/boost_1_50_0.tar.bz2/download -O boost_1_50_0.tar.bz2
elif test -x "/usr/bin/curl"; then
	curl -L --max-redirs 10 http://sourceforge.net/projects/boost/files/boost/1.50.0/boost_1_50_0.tar.bz2/download > boost_1_50_0.tar.bz2
fi
tar -jxvf boost_1_50_0.tar.bz2 boost_1_50_0/boost
cp -avf boost_1_50_0/boost include/
rm -rfv boost*
