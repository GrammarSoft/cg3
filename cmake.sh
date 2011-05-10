echo "- rm -rf CMake caches" && \
rm -rf CMakeCache.txt cmake_install.cmake CMakeFiles src/CMakeFiles src/cmake_install.cmake && \
echo "- cmake Release ." && \
cmake -DCMAKE_BUILD_TYPE=Release . && \
echo "- You may now perform: make -j3"
