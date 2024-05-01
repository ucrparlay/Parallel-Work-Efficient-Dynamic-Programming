
This repo contains code for our paper "Parallel and (Nearly) Work-Efficient Dynamic Programming".

## Requirements

- CMake >= 3.15
- g++ or clang with C++20 features support (tested with g++ 12.1.1 and clang 14.0.6) on Linux machines.

## Download Code

```
git clone --recurse-submodules git@github.com:ucrparlay/Parallel-Work-Efficient-Dynamic-Programming.git
```

## Compilation

```
mkdir build && cd build
cmake ..
make
```

## Running Code

Run the parallel convex GLWS (post office problem):
```
./post_office -n <# of villages> -range <range of coordinates> -cost <cost per post office>
```

Run the parallel LCS:
```
./lcs -n <input string length> -m <# of pair matches> -k <LCS length>
```
