echo "NOTICE: This script is deprecated."
echo "Instead, run autogen.sh or configure, then make, ./test/runall.pl, and if Success, make install."
g++ -O2 -Wall -ffor-scope -ffast-math -fexpensive-optimizations -licuio -licuuc -licui18n -licudata $(ls -1 ./src/*.cpp | egrep -v '/cg_' | grep -v Apertium) -o vislcg3
strip vislcg3
