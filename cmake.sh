echo "- rm -rf CMake caches" && \
rm -rf install_manifest.txt CMakeCache.txt *.cmake CMakeFiles src/CMakeFiles src/*.cmake _CPack_Packages && \
echo "- cmake" "$@" "." && \
cmake "$@" . && \
echo "- You may now perform: make -j3"
