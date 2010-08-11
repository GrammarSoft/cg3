#!/bin/bash
mkdir -p vparse

svn up
cd vparse
rm -f callgrind.*
rm -f annotated
g++ -Wall -Wextra -Wno-deprecated -O3 -g3 -fno-rtti -ffor-scope -licuio -licuuc $(ls -1 ../src/*.cpp | egrep -v '/test_' | egrep -v '/cg_' | grep -v Apertium | grep -v Matxin | grep -v FormatConverter) -o vislcg3.debug
valgrind --tool=callgrind --compress-strings=no --compress-pos=no --collect-jumps=yes --collect-systime=yes ./vislcg3.debug -C ISO-8859-1 --grammar-only -g ~/parsers/dansk/etc/dancg
callgrind_annotate --tree=both > annotated
