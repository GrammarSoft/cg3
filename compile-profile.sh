mkdir -p gcov
rm -f gcov/*
cd gcov
g++ -Wall -O3 -g3 -fprofile-arcs -ftest-coverage -ffor-scope -licuio -licuuc ../src/*.cpp -o vislcg3
cd ..
mkdir -p gprof
rm -f gprof/*
cd gprof
g++ -pg -Wall -O3 -g3 -ffor-scope -licuio -licuuc ../src/*.cpp -o vislcg3
