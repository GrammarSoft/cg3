#!/bin/bash
mkdir -p gprof
mkdir -p gcov

svn up
make -j3
./src/vislcg3 -C UTF-8 -g ~/parsers/dansk/etc/dancg --grammar-only --grammar-bin dancg.bin3
./compile-profile.sh

cd gcov/
#head -n 3000 arboretum_stripped.txt | ../src/vislcg3 -v -C UTF-8 -g ../dancg.bin3 > out1.txt
head -n 3000 arboretum_stripped.txt | ./vislcg3 -v -C UTF-8 -g ../dancg.bin3 > out2.txt
find ../src/ -type f -name '*.cpp' -exec gcov -o . {} \; >/dev/null 2>/dev/null
gcov -o . ../src/GrammarApplicator_runRules.cpp  >/dev/null 2>/dev/null
gcov -o . ../src/GrammarApplicator_matchSet.cpp  >/dev/null 2>/dev/null

cd ../gprof/
head -n 3000 arboretum_stripped.txt | ./vislcg3 -v -C UTF-8 -g ../dancg.bin3 > out3.txt
gprof vislcg3 >flat.txt
