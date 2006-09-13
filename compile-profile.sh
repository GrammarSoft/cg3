mkdir -p gcov
cd gcov
g++ -Wall -O0 -g3 -fprofile-arcs -ftest-coverage -ffor-scope -licuio -licuuc ../src/*.cpp -o vislcg3
cd ..
mkdir -p gprof
cd gprof
g++ -pg -Wall -O0 -g3 -ffor-scope -licuio -licuuc ../src/*.cpp -o vislcg3
