#!/bin/bash
# 98 or 11
CXXV=11
#svn up
mkdir -p vparse
cd vparse
rm -f callgrind.*
mv -vf annotated annotated.old
g++ -std=c++$CXXV -DHAVE_BOOST -I~/tmp/boost_1_54_0 -Wall -Wextra -Wno-deprecated -Wno-unused-local-typedefs -pipe -O3 -g3 -L/usr/local/lib64 -L/usr/local/lib -licuio -licuuc -I../include -I../include/exec-stream ../src/all_vislcg3.cpp -o vislcg3-c++$CXXV.debug
valgrind --tool=callgrind --compress-strings=no --compress-pos=no --collect-jumps=yes --collect-systime=yes ./vislcg3-c++$CXXV.debug -C UTF-8 --grammar-only -g ~/parsers/dansk/etc/dancg
callgrind_annotate --tree=both --auto=yes > annotated
