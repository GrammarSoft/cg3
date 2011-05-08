echo "- rm -rf CMake caches" && \
rm -rf CMakeCache.txt cmake_install.cmake CMakeFiles src/CMakeFiles src/cmake_install.cmake && \
echo "- cmake ." && \
cmake . && \
echo "You may now perform: make -j3"
