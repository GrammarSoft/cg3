#!/bin/bash
#svn up
mkdir -p vparse
cd vparse
rm -f callgrind.*
mv -vf annotated annotated.old
g++ -std=c++14 -DNDEBUG -Wall -Wextra -Wno-deprecated -pthread -pipe -O3 -g3 -I../include -I../include/posix ../src/all_vislcg3.cpp -o vislcg3.debug -L/usr/lib/x86_64-linux-gnu -licui18n -licudata -licuio -licuuc
valgrind --tool=callgrind --compress-strings=no --compress-pos=no --collect-jumps=yes --collect-systime=yes ./vislcg3.debug --grammar-only -g ~/parsers/dansk/etc/dancg.cg
callgrind_annotate --tree=both --auto=yes > annotated
