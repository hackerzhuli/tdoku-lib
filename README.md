## Tdoku: A fast Sudoku Solver and Generator Library

#### Overview
This project contains an optimized Sudoku solver and puzzle generator for conventional 9x9 puzzles (as well as Sukaku
"pencilmark" puzzles with clues given as negative instead of positive literals).

Note: Tdoku makes heavy use of SIMD instructions up to various flavors of AVX-512 when available. As a result
it achieves its best performance on recent Intel hardware like the Ice Lake laptop. With older processors there are moderate declines in performance down to SSSE3, and
precipitous declines with SSE2.

#### Building and Running

Build this project's libraries tools and tests as follows:

```bash
./BUILD.sh
./build/run_tests
```

You can build a 
simple test program that reads Sudoku (or 729-character pencilmark Sudoku) from stdin and displays 
the solution count and solution (if unique) like so:

```bash
# if you haven't done so, unzip the data
unzip data.zip
gcc example/solve.c build/libtdoku_static.a -O3 -o solve -lstdc++ -lm
# count solutions:
./solve < data/puzzles0_kaggle
# find single solution:
./solve 1 < data/puzzles0_kaggle
```

Or for an example of using the shared library via python bindings try:

```bash
# if you haven't done so, unzip the data
unzip data.zip
python3 example/solve.py data/puzzles0_kaggle
```


#### Origin
This Project is forked from [tdoku](https://github.com/t-dillon/tdoku/tree/master). The purpose is to contain the generator into the tdoku library, instead of an executable. So it can be easily used with an c interface.

Also, benchmarks, other solvers and docs are deleted so it is simpler for users who just want to use the tdoku library.

