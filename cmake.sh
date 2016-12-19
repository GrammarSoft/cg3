#!/usr/bin/env bash
set -e
args=()

while [[ $# > 0 ]];
do
	case "$1" in
	--prefix)
		args+=("-DCMAKE_INSTALL_PREFIX=$2")
		shift 2
		;;
	--prefix=*)
		args+=("-DCMAKE_INSTALL_PREFIX=${1#*=}")
		shift
		;;
	*)
		args+=("$1")
		shift
		;;
	esac
done

set -- "${args[@]}"

echo "- rm -rf CMake caches"
rm -rf install_manifest.txt CMakeCache.txt *.cmake CMakeFiles src/CMakeFiles src/*.cmake _CPack_Packages Testing test/T_*/grammar.cg3b
echo "- cmake " "$@" "."
cmake "$@" .
echo "- You may now perform: make -j3"
