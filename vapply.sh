#!/bin/bash
# 98 or 11
CXXV=11
#svn up
make -j5
./src/vislcg3 -C UTF-8 -g ~/parsers/dansk/etc/dancg --grammar-only --grammar-bin dancg.cg3b
mkdir -p vapply
cd vapply
rm -fv callgrind.*
mv -vf annotated annotated.old
g++ -std=c++$CXXV -DHAVE_BOOST -I~/tmp/boost_1_54_0 -pipe -Wall -Wextra -Wno-deprecated -Wno-unused-local-typedefs -O3 -g3 -L/usr/local/lib64 -L/usr/local/lib -licuio -licuuc -I../include -I../include/exec-stream ../src/all_vislcg3.cpp -o vislcg3-c++$CXXV.debug
head -n 2000 ../comparison/arboretum_stripped.txt | valgrind --tool=callgrind --compress-strings=no --compress-pos=no --collect-jumps=yes --collect-systime=yes ./vislcg3-c++$CXXV.debug -v -C UTF-8 -g ../dancg.cg3b > output.txt
#head -n 2000 ../comparison/arboretum_stripped.txt | valgrind ./vislcg3.debug -v -C UTF-8 -g ../dancg.cg3b > output.txt
callgrind_annotate --tree=both --auto=yes > annotated
