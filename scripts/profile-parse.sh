#!/bin/bash
# 98 or 11
export CXXV=11
mkdir -p gprof
mkdir -p gcov

svn up
./compile-profile.sh

cd gcov/
./vislcg3-c++$CXXV -v0 -C UTF-8 -g ~/parsers/dansk/etc/dancg --grammar-only --grammar-bin dancg.cg3b
gcov -o . ../src/*.cpp >/dev/null 2>/dev/null
gcov -o . ../src/TextualParser.cpp  >/dev/null 2>/dev/null

cd ../gprof/
./vislcg3-c++$CXXV -v0 -C UTF-8 -g ~/parsers/dansk/etc/dancg --grammar-only --grammar-bin dancg.cg3b
gprof vislcg3 >flat.txt
