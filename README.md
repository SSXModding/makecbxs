## Makecbxs

Tool to automatically uncompress & dump all of the files in BAM.[x]SB in SSX 3.


## Usage:

`path to makecbxs  /path/to/data/worlds/bam.[x]sb output_path_name_here`

## Building

All you need is a decent C++17 compiler. Makecbxs should compile and run everywhere with a C++ standard library

```
mkdir build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j3 (or cmake --build .)
```