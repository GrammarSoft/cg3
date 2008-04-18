mkdir -p gcov
rm -f gcov/*
cd gcov
g++ -Wall -O2 -g3 -fprofile-arcs -ftest-coverage -ffor-scope -licuio -licuuc $(ls -1 ../src/*.cpp | egrep -v '/cg_') -o vislcg3
cd ..
mkdir -p gprof
rm -f gprof/*
cd gprof
g++ -pg -Wall -O2 -g3 -ffor-scope -licuio -licuuc $(ls -1 ../src/*.cpp | egrep -v '/cg_') -o vislcg3
