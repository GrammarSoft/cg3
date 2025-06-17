set -eox pipefail

# To run this script in a Docker container, use the following command:
# docker run -it -v $(pwd):/cg3 ubuntu

if ! [ -f src/cg3.h ]; then
    echo "This script must be run from the root directory of the CG3 project."
    exit 1
fi

CG3_ROOT=$(pwd)

if command -v sudo >/dev/null 2>&1; then
    SUDO="sudo"
else
    SUDO=""
fi

"${SUDO}" apt-get update
"${SUDO}" apt-get install -qy apt-utils build-essential cmake git libtool python3 wget
# "${SUDO}" apt-get install -qy vim less tree

if ! [ -d /tmp/rapidjson ]; then
    git clone --depth 1 https://github.com/Tencent/rapidjson.git /tmp/rapidjson
fi
if ! [ -f /tmp/rapidjson/RapidJSONConfig.cmake ]; then
    pushd /tmp/rapidjson
    cmake .
    make
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
LDFLAGS="-sMODULARIZE=1 \
         -sEXPORT_NAME='createCG3Module' \
         -sDYNAMIC_EXECUTION=0 \
         -sALLOW_MEMORY_GROWTH=1 \
         -sFORCE_FILESYSTEM=1 \
         -sASSERTIONS=1 \
         -sENVIRONMENT=web"
export LDFLAGS
export CPPFLAGS="-I/usr/local/include"
export CXXFLAGS="-I/usr/local/include"
# To build with debugging symbols, use...
#    -DCMAKE_BUILD_TYPE=Debug \
emcmake cmake \
    -DRapidJSON_DIR=/usr/local/lib/cmake/RapidJSON \
    ..
time emmake make V=1 VERBOSE=1

# Show the build artifacts
cd ${CG3_ROOT}
echo "Build artifacts:"
find . -type f \( -name "*.js" -o -name "*.wasm*" \)
