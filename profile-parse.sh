#!/bin/bash
mkdir -p gprof
mkdir -p gcov

svn up
./compile-profile.sh

cd gcov/
./vislcg3 -v0 -C UTF-8 -g ~/parsers/dansk/etc/dancg --grammar-only --grammar-bin dancg.bin3
gcov -o . ../src/*.cpp >/dev/null 2>/dev/null
gcov -o . ../src/TextualParser.cpp  >/dev/null 2>/dev/null

cd ../gprof/
./vislcg3 -v0 -C UTF-8 -g ~/parsers/dansk/etc/dancg --grammar-only --grammar-bin dancg.bin3
gprof vislcg3 >flat.txt
