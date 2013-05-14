#!/bin/bash
#svn up
mkdir -p vparse
cd vparse
rm -f callgrind.*
rm -f annotated
g++ -DHAVE_BOOST -I~/tmp/boost_1_49_0 -Wall -Wextra -Wno-deprecated -pipe -O3 -g3 -fno-rtti -ffor-scope -L/usr/local/lib -ltcmalloc -licuio -licuuc ../src/all_vislcg3.cpp -o vislcg3.debug
valgrind --tool=callgrind --compress-strings=no --compress-pos=no --collect-jumps=yes --collect-systime=yes ./vislcg3.debug -C UTF-8 --grammar-only -g ~/parsers/dansk/etc/dancg
callgrind_annotate --tree=both --auto=yes > annotated
