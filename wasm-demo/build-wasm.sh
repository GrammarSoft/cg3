#!/bin/bash
set -eox pipefail

# To run this script in a Docker container, use the following command:
# docker run -it -v $(pwd):/cg3 ubuntu

if ! [ -f src/cg3.h ]; then
	echo "This script must be run from the root directory of the CG3 project."
	exit 1
fi

# Check if we are running in a Docker container
if [ -f /.dockerenv ]; then
	echo "Running in a Docker container. Proceeding with build."
else
	echo "This script is intended to be run in a Debian/Ubuntu container."
	read -p "Do you want to continue? (y/n) " answer
	if [[ ! $answer =~ ^[Yy]$ ]]; then
		echo "Exiting."
		exit 1
	fi
fi

CG3_ROOT=$(pwd)

SUDO=
if command -v sudo >/dev/null 2>&1; then
	SUDO="sudo"
fi

${SUDO} apt-get update
${SUDO} apt-get install -qfy --no-install-recommends apt-utils build-essential cmake git libtool python3 wget ca-certificates

if ! [ -d /tmp/rapidjson ]; then
	git clone --depth 1 https://github.com/Tencent/rapidjson.git /tmp/rapidjson
fi
if ! [ -f /usr/local/lib/cmake/RapidJSON/RapidJSONConfig.cmake ]; then
	pushd /tmp/rapidjson
	cmake .
	make -j
	${SUDO} make install
	popd
fi

if ! [ -d /tmp/emsdk ]; then
	git clone --depth 1 https://github.com/emscripten-core/emsdk.git /tmp/emsdk
fi
pushd /tmp/emsdk
./emsdk install latest
./emsdk activate latest
source emsdk_env.sh
popd

mkdir -p wasm-build
cd wasm-build
rm -rf ../CMakeCache.txt ../CMakeFiles CMakeCache.txt src/libcg3.*
export LDFLAGS="-sMODULARIZE=1 \
		 -sEXPORT_NAME='createCG3Module' \
		 -sDYNAMIC_EXECUTION=0 \
		 -sALLOW_MEMORY_GROWTH=1 \
		 -sFORCE_FILESYSTEM=1 \
		 -sASSERTIONS=1 \
		 -sENVIRONMENT=web"
export CPPFLAGS="-I/usr/local/include"
export CXXFLAGS="-I/usr/local/include"
# To build with debugging symbols, use...
#	-DCMAKE_BUILD_TYPE=Debug \
emcmake cmake \
	-DRapidJSON_DIR=/usr/local/lib/cmake/RapidJSON \
	..
time emmake make -j V=1 VERBOSE=1

# Show the build artifacts
cd ${CG3_ROOT}
ln -sf ../wasm-build/src/libcg3.js wasm-demo/libcg3.js
ln -sf ../wasm-build/src/libcg3.wasm wasm-demo/libcg3.wasm

echo "Build artifacts:"
find . -type f \( -name "*.js" -o -name "*.wasm*" \)
