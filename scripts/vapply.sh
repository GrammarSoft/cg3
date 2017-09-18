#!/bin/bash
#svn up
make -j5
./src/vislcg3 -g ~/parsers/dansk/etc/dancg.cg --grammar-only --grammar-bin dancg.cg3b
mkdir -p vapply
cd vapply
rm -fv callgrind.*
mv -vf annotated annotated.old
mv -vf output.txt output.txt.old
g++ -std=c++14 -DNDEBUG -pthread -pipe -Wall -Wextra -Wno-deprecated -O3 -g3 -I../include -I../include/posix ../src/all_vislcg3.cpp -o vislcg3.debug -L/usr/lib/x86_64-linux-gnu -licui18n -licudata -licuio -licuuc
head -n 2000 ../comparison/arboretum_stripped.txt | valgrind --tool=callgrind --compress-strings=no --compress-pos=no --collect-jumps=yes --collect-systime=yes ./vislcg3.debug -v -t -g ../dancg.cg3b > output.txt
callgrind_annotate --tree=both --auto=yes > annotated
