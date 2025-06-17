# CG3 WebAssembly Demo

This directory contains examples of how to build and implement a WebAssembly build of the CG3 library. The build script is intended to be run from the project root directory using a Docker container. This ensures a consistent build environment with all required dependencies (without installing unwanted dependencies on your host machine).

### Prerequisites

- Docker installed on your system

### Build Instructions

1. Navigate to the project root directory (not this wasm-demo directory):
2. Run the build script in a Docker container:
   ```bash
   docker run -it -v $(pwd):/cg3 -w /cg3 ubuntu bash wasm-demo/build-wasm.sh
   ```

The build script will:
- Install required dependencies (emscripten, cmake, etc.)
- Clone and build RapidJSON
- Set up the Emscripten SDK
- Build the CG3 library as a WebAssembly module

The build process generates the following files:
- `*.js` - JavaScript module files
- `*.wasm` - WebAssembly binary files

These files are automatically found and listed by the build script.

## Running the Demo

After building, you can run the demo locally:

1. From the project root directory, start a local web server:
   ```bash
   python3 -m http.server
   ```

2. Open your browser and navigate to:
   ```
   http://localhost:8000/wasm-demo
   ```

3. You should see the CG3 WebAssembly demo with example usage of the compiled library.
