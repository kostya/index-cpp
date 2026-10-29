# C++ Stress Test

An amalgamated, simplified version of the [LangArena](https://github.com/kostya/LangArena)
suite for the C++ language.

LangArena is a collection of 50 real-world-inspired tasks: JSON, Base64, CSV,
neural networks, compression, maze A*, graph algorithms, sorting, hashing,
interpreters, parallel matmul, and more.

The file (index.cpp) serves three purposes at once:

- **Compiler stress test** - a single ~350 KB translation unit exercises the
  front end, optimizer, and back end.
- **Runtime benchmark** - each test runs for roughly a second, so the total
  runtime is around 52 seconds on my hardware.
- **Correctness verifier** - every test checks its result against a checksum.
  If any test fails, the program exits with code 1.

Fully "all in": no external dependencies beyond the standard libraries (requires C++17).

All parameters have been tuned for GCC 16 -O2. Timings may differ on other
hardware.

### Build and run:

    g++ -std=c++17 -w -O3 index.cpp -lpthread -o ./index
    ./index

### Fast run, only test checksums

    TEST=1 ./index

### Source includes:

* https://github.com/DaveGamble/cJSON
  MIT License (Copyright (c) 2009-2017 Dave Gamble and cJSON contributors)

* https://github.com/kokke/tiny-regex-c
  All material in this repository is in the public domain.

* https://github.com/wareya/Remimu/
  Creative Commons Legal Code

### MIT License

# Experiment: G++ and Clang++ performance over 10 years.

I measured runtime, compilation time, compilation RSS and binary size for `index.cpp` across G++ 7–16 and Clang++ 7–23, using the flags `-O0`, `-O1`, `-O2`, `-O3`, `-Os`, `-Oz`, `-Ofast`, and `-O3 -march=native`. Experiment date: 2026-10-30.

Environment:

- **CPU:** Ryzen 3800X
- **RAM:** 78 GB DDR4-3200
- **OS:** Linux arch 7.2.7
- **Docker:** 29.8.1

The benchmarks were executed in Docker, with images pulled from Docker Hub. This can introduce uncertainty into the results, since each image may contain its own version of glibc and other unknown factors. GCC builds used the official GCC images; Clang builds used images from teeks99.

**G++:** 7.5.0, 8.5.0, 9.5.0, 10.5.0, 11.5.0, 12.5.0, 13.5.0, 14.4.0, 15.3.0, 16.2.0.

**Clang++:** 7.1.0, 8.0.1, 9.0.0, 10.0.1, 11.1.0, 12.0.1, 13.0.1, 14.0.6, 15.0.7, 16.0.6, 17.0.6, 18.1.8, 19.1.7, 20.1.8, 21.1.8, 22.1.8, 23.1.4.

### Results for `-O1`, `-O2`, `-O3`

![plot](https://raw.githubusercontent.com/kostya/stuff/master/index-cpp/plot1_runtime.png)

![plot](https://raw.githubusercontent.com/kostya/stuff/master/index-cpp/plot1_compile.png)

Regarding the slowdown in G++ compile time (in `-O2`/`-O3` mode), it is clearly reproducible with two simple commands (using the official GCC images):

```
docker run --rm -v `pwd`:/src -w /src gcc:7 bash -c 'time g++ -std=c++17 -w -O3 index.cpp -lpthread -o /tmp/index'
```
real  0m7.378s

```
docker run --rm -v `pwd`:/src -w /src gcc:16 bash -c 'time g++ -std=c++17 -w -O3 index.cpp -lpthread -o /tmp/index'
```
real  0m10.184s

This is also correlated with the results for index-c: https://github.com/kostya/index-c#results-for--o1--o2--o3

### Evolution plot for `-O2`

![plot](https://raw.githubusercontent.com/kostya/stuff/master/index-cpp/plot6.png)

### No optimizations: `-O0`

![plot](https://raw.githubusercontent.com/kostya/stuff/master/index-cpp/plot2_runtime.png)

![plot](https://raw.githubusercontent.com/kostya/stuff/master/index-cpp/plot2_compile.png)

### Compile memory, Mb

![plot](https://raw.githubusercontent.com/kostya/stuff/master/index-cpp/plot7_compile_mem.png)

### Binary size, Kb

![plot](https://raw.githubusercontent.com/kostya/stuff/master/index-cpp/plot3_binary_size.png)

### Results for `-Os`, `-Oz` vs `-O2`

![plot](https://raw.githubusercontent.com/kostya/stuff/master/index-cpp/plot4_runtime.png)

![plot](https://raw.githubusercontent.com/kostya/stuff/master/index-cpp/plot4_compile.png)

### Results for `-Ofast`, `-O3 -march=native` vs `-O3`

![plot](https://raw.githubusercontent.com/kostya/stuff/master/index-cpp/plot5_runtime.png)

![plot](https://raw.githubusercontent.com/kostya/stuff/master/index-cpp/plot5_compile.png)

The tested processor came out in 2019, but the effect of `-march=native` only started to matter in 2022.

* [reproduce script](https://github.com/kostya/index-cpp/blob/master/experiment/run.rb)
* [raw data](https://github.com/kostya/index-cpp/blob/master/experiment/merged.js)


# Related

* [index-c](https://github.com/kostya/index-c) - A single-file, self-contained stress test for C compilers and optimizers.
* [LangArena](https://github.com/kostya/LangArena) - LangArena: Programming Language Evaluation Suite.

