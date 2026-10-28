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

Fully "all in": no external dependencies beyond the standard libraries (require C++17).

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

# Related

* [index-c](https://github.com/kostya/index-c) - A single-file, self-contained stress test for C compilers and optimizers.
* [LangArena](https://github.com/kostya/LangArena) - LangArena: Programming Language Evaluation Suite.

