g++ -combine -O3 -Wall -ffor-scope -ffast-math -floop-optimize -fexpensive-optimizations -fmove-loop-invariants -mmmx -msse -licuio -licuuc src/*.cpp -o vislcg3
strip vislcg3
