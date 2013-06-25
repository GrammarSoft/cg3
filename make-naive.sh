#!/bin/bash
g++ -DHAVE_BOOST -pipe -Wall -Wextra -Wno-deprecated -O3 -fno-rtti -ffor-scope -licuio -licuuc ./src/all_vislcg3.cpp -o vislcg3
g++ -DHAVE_BOOST -pipe -Wall -Wextra -Wno-deprecated -O3 -fno-rtti -ffor-scope -licuio -licuuc ./src/all_cg-comp.cpp -o cg-comp
g++ -DHAVE_BOOST -pipe -Wall -Wextra -Wno-deprecated -O3 -fno-rtti -ffor-scope -licuio -licuuc ./src/all_cg-proc.cpp -o cg-proc
g++ -DHAVE_BOOST -pipe -Wall -Wextra -Wno-deprecated -O3 -fno-rtti -ffor-scope -licuio -licuuc ./src/all_cg-conv.cpp -o cg-conv
