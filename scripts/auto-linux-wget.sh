#!/bin/bash

echo "Cleaning up /tmp/vislcg3-auto ..."
cd /tmp/
rm -rfv vislcg3-auto
mkdir vislcg3-auto
cd vislcg3-auto

echo "Fetching VISL CG-3 source with wget..."
wget -nv -r -l0 -np -nH --http-user=anonymous --http-passwd=anonymous http://beta.visl.sdu.dk/svn/visl/tools/vislcg3/trunk/
cd svn/visl/tools/vislcg3/trunk/
chmod +x *.sh test/*.pl test/*/*.pl

echo "Fetched everything...compiling..."
./compile-linux.sh

echo "Compiled. Running regression tests..."
./test/runall.pl

echo "If all normal tests were successful, you can now do:"
echo "cp $(pwd)/vislcg3 /usr/local/bin/"
