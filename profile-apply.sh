#!/bin/bash
# 98 or 11
export CXXV=11
mkdir -p gprof
mkdir -p gcov

svn up
make -j5
./src/vislcg3 -C UTF-8 -g ~/parsers/dansk/etc/dancg --grammar-only --grammar-bin dancg.cg3b
./compile-profile.sh

cd gcov/
#head -n 3000 arboretum_stripped.txt | ../src/vislcg3-c++$CXXV -v -C UTF-8 -g ../dancg.cg3b > out1.txt
head -n 3000 arboretum_stripped.txt | ./vislcg3-c++$CXXV -v -C UTF-8 -g ../dancg.cg3b > out2.txt
find ../src/ -type f -name '*.cpp' -exec gcov -o . {} \; >/dev/null 2>/dev/null
gcov -o . ../src/GrammarApplicator_runRules.cpp  >/dev/null 2>/dev/null
gcov -o . ../src/GrammarApplicator_matchSet.cpp  >/dev/null 2>/dev/null

cd ../gprof/
head -n 3000 arboretum_stripped.txt | ./vislcg3-c++$CXXV -v -C UTF-8 -g ../dancg.cg3b > out3.txt
gprof vislcg3 >flat.txt
