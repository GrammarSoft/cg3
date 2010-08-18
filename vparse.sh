#!/bin/bash
#svn up
mkdir -p vparse
cd vparse
rm -f callgrind.*
rm -f annotated
g++ -Wall -Wextra -Wno-deprecated -O3 -g3 -fno-rtti -ffor-scope -licuio -licuuc ../src/all_vislcg3.cpp -o vislcg3.debug
valgrind --tool=callgrind --compress-strings=no --compress-pos=no --collect-jumps=yes --collect-systime=yes ./vislcg3.debug -C ISO-8859-1 --grammar-only -g ~/parsers/dansk/etc/dancg
callgrind_annotate --tree=both > annotated
