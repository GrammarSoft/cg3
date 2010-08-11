#!/bin/bash
mkdir -p vapply

svn up
make -j3
./src/vislcg3 -C UTF-8 -g ~/parsers/dansk/etc/dancg --grammar-only --grammar-bin dancg.bin3

cd vapply
rm -fv callgrind.*
rm -fv annotated
g++ -DHAVE_BOOST -I~/tmp/boost_1_42_0 -pipe -Wall -Wextra -Wno-deprecated -O3 -g3 -fno-rtti -ffor-scope -licuio -licuuc $(ls -1 ../src/*.cpp | egrep -v '/test_' | egrep -v '/cg_' | grep -v Apertium | grep -v Matxin | grep -v FormatConverter) -o vislcg3.debug
head -n 2000 ../comparison/arboretum_stripped.txt | valgrind --tool=callgrind --compress-strings=no --compress-pos=no --collect-jumps=yes --collect-systime=yes ./vislcg3.debug -v -C UTF-8 -g ../dancg.bin3 > output.txt
#head -n 2000 ../comparison/arboretum_stripped.txt | valgrind ./vislcg3.debug -v -C UTF-8 -g ../dancg.bin3 > output.txt
callgrind_annotate --tree=both > annotated
