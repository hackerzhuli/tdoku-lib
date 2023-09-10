## Tdoku: A fast Sudoku Solver and Generator

#### Overview
This project contains an optimized Sudoku solver and puzzle generator for conventional 9x9 puzzles (as well as Sukaku
"pencilmark" puzzles with clues given as negative instead of positive literals). It also contains two 
other solvers with several variations exploring different ideas for optimization visited during
development.

See [https://t-dillon.github.io/tdoku](https://t-dillon.github.io/tdoku) for a long discussion of the thinking behind these solvers.

This project's solvers include:

Solver | Description
-------|------------
[src/solver_dpll_triad_simd.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_dpll_triad_simd.cc) | The fast solver, optimized with particular focus on speed of solving hard Sudoku instances, and by this measure the fastest of any solver I'm aware of. I'll refer to this one as "Tdoku".
[src/solver_dpll_triad_scc.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_dpll_triad_scc.cc) | A DPLL-based solver for exploring how the puzzle representation can be optimized and strongly connected components can be exploited to reduce backtracking.
[src/solver_basic.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_basic.cc) | A very simple solver. Fast as simple solvers go, but mainly here to illustrate how poorly such solvers handle hard puzzles.

And here is a chart comparing a narrower set of the fastest solvers on a wider range of datasets
ordered roughly from easiest to hardest, and for each solver using the results from its most 
favorable tested compiler and compiler options:

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vRrWT05pUsB0LRS8ZR-j7WNvoUIpX6TDHBGeWhJnd7bRedgNn-a60TLVIRYO9A51yUZuXo-ugWx-ibK/pubchart?oid=1741583019&format=image)

Note: Tdoku makes heavy use of SIMD instructions up to various flavors of AVX-512 when available. As a result
it achieves its best performance on recent Intel hardware like the Ice Lake laptop that produced the tables
and chart above. With older processors there are moderate declines in performance down to SSSE3, and
precipitous declines with SSE2. See the comment [here](https://github.com/t-dillon/tdoku/blob/master/src/simd_vectors.h)
for stats on the availability of SIMD instructions from the Steam hardware survey.

For configuration and full details of the runs used for the comparisons shown above, [see here](https://github.com/t-dillon/tdoku/tree/master/benchmarks/results_i7-1065G7/i7-1065G7_clang-11_O3_native).


#### Building and Running

Build this project's solvers and run them through tests as follows:

```bash
unzip data.zip
./BUILD.sh
./build/run_tests
```
Building the project also produces a library containing the fast simd solver.  You can build a 
simple test program that reads Sudoku (or 729-character pencilmark Sudoku) from stdin and displays 
the solution count and solution (if unique) like so:

```bash
gcc example/solve.c build/libtdoku_static.a -O3 -o solve -lstdc++ -lm
# count solutions:
./solve < data/puzzles0_kaggle
# find single solution:
./solve 1 < data/puzzles0_kaggle
```

Or for an example of using the shared library via python bindings try:

```bash
python3 example/solve.py data/puzzles0_kaggle
```

