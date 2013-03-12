#!/bin/sh
cd "`dirname "$0"`"
rm -rfv boost*
if test -x "/usr/bin/wget"; then
	wget http://sourceforge.net/projects/boost/files/boost/1.53.0/boost_1_53_0.tar.bz2/download -O boost_1_53_0.tar.bz2
elif test -x "/usr/bin/curl"; then
	curl -L --max-redirs 10 http://sourceforge.net/projects/boost/files/boost/1.53.0/boost_1_53_0.tar.bz2/download > boost_1_53_0.tar.bz2
fi
tar -jxvf boost_1_53_0.tar.bz2 boost_1_53_0/boost ./boost_1_53_0/boost
rm -rfv include/boost
mv -vf boost_1_53_0/boost include/
rm -rfv boost*
