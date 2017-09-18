#!/bin/bash
mkdir -p gprof
mkdir -p gcov

svn up
./compile-profile.sh

cd gcov/
./vislcg3 -v0 -g ~/parsers/dansk/etc/dancg.cg --grammar-only --grammar-bin dancg.cg3b
gcov -o . ../src/*.cpp >/dev/null 2>/dev/null
gcov -o . ../src/TextualParser.cpp  >/dev/null 2>/dev/null

cd ../gprof/
./vislcg3 -v0 -g ~/parsers/dansk/etc/dancg.cg --grammar-only --grammar-bin dancg.cg3b
gprof vislcg3 >flat.txt
