mkdir -p gcov
rm -fv gcov/*.gcov
rm -fv gcov/*.gcda
rm -fv gcov/*.gcno
rm -fv gcov/vislcg3
cd gcov
g++ -std=c++14 -DHAVE_BOOST -pthread -pipe -Wall -Wextra -Wno-deprecated -Wno-unused-local-typedefs -O3 -g3 -fprofile-arcs -ftest-coverage -I../include -I../include/posix ../src/all_vislcg3.cpp -o vislcg3 -L/usr/lib/x86_64-linux-gnu -licui18n -licudata -licuio -licuuc -ltcmalloc
cd ..
mkdir -p gprof
rm -fv gprof/vislcg3
rm -fv gprof/gmon.out
cd gprof
g++ -std=c++14 -DHAVE_BOOST -pthread -pipe -pg -Wall -Wextra -Wno-deprecated -Wno-unused-local-typedefs -O3 -g3 -I../include -I../include/posix ../src/all_vislcg3.cpp -o vislcg3 -L/usr/lib/x86_64-linux-gnu -licui18n -licudata -licuio -licuuc -ltcmalloc
