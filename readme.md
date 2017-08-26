# About
bunch of snippets and small experiments with c++

# Compile
## Linux & Macos
Somewhere in your filesystem (but not in the project folder, cause in-source builds are for retards):
```bash
mkdir build
cd build
cmake /path/to/this/project/source/dir [-DCMAKE_INSTALL_PREFIX=/install/prefix/path]
cmake --build .
cmake --build -- install .
```

## Dependencies
### Required:
- CMake
### Optional:
- [catch](https://github.com/philsquared/Catch) (for unit tests).
- [benchmark](https://github.com/google/benchmark) for benchmarking

# License
Do the fuck you want with it.
