mkdir -p gcov
rm -fv gcov/*.gcov
rm -fv gcov/*.gcda
rm -fv gcov/*.gcno
rm -fv gcov/vislcg3-c++$CXXV
cd gcov
g++ -std=c++$CXXV -DHAVE_BOOST -I~/tmp/boost_1_54_0 -pipe -Wall -Wextra -Wno-deprecated -Wno-unused-local-typedefs -O3 -g3 -fprofile-arcs -ftest-coverage -L/usr/local/lib64 -L/usr/local/lib -ltcmalloc -licuio -licuuc -I../include -I../include/exec-stream ../src/all_vislcg3.cpp -o vislcg3-c++$CXXV
cd ..
mkdir -p gprof
rm -fv gprof/vislcg3-c++$CXXV
rm -fv gprof/gmon.out
cd gprof
g++ -std=c++$CXXV -DHAVE_BOOST -I~/tmp/boost_1_54_0 -pipe -pg -Wall -Wextra -Wno-deprecated -Wno-unused-local-typedefs -O3 -g3 -L/usr/local/lib64 -L/usr/local/lib -ltcmalloc -licuio -licuuc -I../include -I../include/exec-stream ../src/all_vislcg3.cpp -o vislcg3-c++$CXXV
