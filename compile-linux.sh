echo "NOTICE: This script is deprecated."
echo "Instead, run autogen.sh or configure, then make, ./test/runall.pl, and if Success, make install."
g++ -O2 -Wall -funroll-loops -ffor-scope -ffast-math -fexpensive-optimizations -licuio -licuuc $(ls -1 ./src/*.cpp | egrep -v '/test_' | egrep -v '/cg_' | grep -v Apertium | grep -v Matxin) -o vislcg3
strip vislcg3
