#!/bin/bash
mkdir -p gprof
mkdir -p gcov

svn up
make -j5
./src/vislcg3 -g ~/parsers/dansk/etc/dancg.cg --grammar-only --grammar-bin dancg.cg3b
./scripts/compile-profile.sh

cd gcov/
head -n 3000 arboretum_stripped.txt | ./vislcg3 -v -g ../dancg.cg3b > out.txt
find ../src/ -type f -name '*.cpp' -exec gcov -o . {} \; >/dev/null 2>/dev/null
gcov -o . ../src/GrammarApplicator_runRules.cpp  >/dev/null 2>/dev/null
gcov -o . ../src/GrammarApplicator_matchSet.cpp  >/dev/null 2>/dev/null

cd ../gprof/
head -n 3000 arboretum_stripped.txt | ./vislcg3 -v -g ../dancg.cg3b > out.txt
gprof vislcg3 >flat.txt
