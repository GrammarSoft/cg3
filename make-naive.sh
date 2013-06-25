#!/bin/bash
echo "Building vislcg3 ..."
g++ -DHAVE_BOOST -pipe -Wall -Wextra -Wno-deprecated -O3 -fno-rtti -ffor-scope -licuio -licuuc $@ ./src/all_vislcg3.cpp -o vislcg3
echo "Building cg-comp ..."
g++ -DHAVE_BOOST -pipe -Wall -Wextra -Wno-deprecated -O3 -fno-rtti -ffor-scope -licuio -licuuc $@ ./src/all_cg_comp.cpp -o cg-comp
echo "Building cg-proc ..."
g++ -DHAVE_BOOST -pipe -Wall -Wextra -Wno-deprecated -O3 -fno-rtti -ffor-scope -licuio -licuuc $@ ./src/all_cg_proc.cpp -o cg-proc
echo "Building cg-conv ..."
g++ -DHAVE_BOOST -pipe -Wall -Wextra -Wno-deprecated -O3 -fno-rtti -ffor-scope -licuio -licuuc $@ ./src/all_cg_conv.cpp -o cg-conv
