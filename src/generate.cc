#include "../include/tdoku.h"
#include "klib/ketopt.h"
#include "util.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <tuple>
#include <vector>

using namespace std;

extern "C" {
size_t OtherSolverMiniSat(const char *input,
                          size_t /*unused_limit*/,
                          uint32_t configuration,
                          char *solution, size_t *num_guesses);
}

extern "C" {
size_t OtherSolverGurobi(const char *input,
                         size_t limit,
                         uint32_t configuration,
                         char *solution, size_t *num_guesses);
}

struct Options {
    uint32_t max_puzzles = 1;
    double clue_weight = 1.0;
    double guess_weight = 0.5;
    double random_weight = 1.0;
    int clues_to_drop = 3;
    int num_evals = 10;
    //int num_puzzles_in_pool = 500;
    uint64_t random_seed = 0; // 0 is ignored
    bool minimize = true;
    bool pencilmark = false;
};

struct Generator {
    Options options_;
    Util util_{};
    char* pattern_list;
    const size_t max_pattern = 64;
    size_t pattern_size = 81;

    explicit Generator(const Options &options) : options_(options) {
        if(options.random_seed != 0){
            util_.RandomSeed(options.random_seed);
        }
        if(options.pencilmark){
            pattern_size = 729;
        }
        pattern_list = new char[max_pattern * pattern_size];
    }

    ~Generator(){
        delete[] pattern_list;
    }

    const string kInitPencilmark =
            "123456789123456789123456789123456789123456789123456789123456789123456789123456789"
            "123456789123456789123456789123456789123456789123456789123456789123456789123456789"
            "123456789123456789123456789123456789123456789123456789123456789123456789123456789"
            "123456789123456789123456789123456789123456789123456789123456789123456789123456789"
            "123456789123456789123456789123456789123456789123456789123456789123456789123456789"
            "123456789123456789123456789123456789123456789123456789123456789123456789123456789"
            "123456789123456789123456789123456789123456789123456789123456789123456789123456789"
            "123456789123456789123456789123456789123456789123456789123456789123456789123456789"
            "123456789123456789123456789123456789123456789123456789123456789123456789123456789";

    const string kInitVanilla =
            ".................................................................................";

    void InitEmpty() {
        const string &initial = options_.pencilmark ? kInitPencilmark : kInitVanilla;

        for (int i = 0; i < max_pattern; i++) {
            memcpy(pattern_list + pattern_size * i, initial.c_str(), pattern_size);
        }
    }

    bool HasUniqueSolution(const char *puzzle) {
        char solution[81];
        size_t guesses = 0;
        return TdokuSolverDpllTriadSimd(puzzle, 2, 0, solution, &guesses) == 1;
    }

    size_t Generate(char* output_puzzles, char separator) {
        char puzzle[730];
        char pattern[730];
        uint32_t count = 0;

        size_t size = pattern_size;
        for (uint64_t i = 0; i < options_.max_puzzles; i++) {
            // draw a pattern from the pool
            size_t which = util_.RandomUInt() % max_pattern;
            memcpy(pattern, pattern_list + size * which, size);
            memcpy(puzzle, pattern, size);
            puzzle[size] = '\0';

            // randomly drop clues to unconstrain
            int dropped = 0;
            for (int j : util_.Permutation(size)) {
                if (dropped == options_.clues_to_drop) {
                    break;
                }
                if (puzzle[j] == '.') {
                    if (options_.pencilmark) {
                        puzzle[j] = (char) ('1' + (j % 9));
                        dropped++;
                    }
                } else {
                    if (!options_.pencilmark) {
                        puzzle[j] = '.';
                        dropped++;
                    }
                }
            }

            // randomly complete and minimize
            if (options_.clues_to_drop > 0) {
                if (!TdokuConstrain(options_.pencilmark, puzzle)) {
                    continue;
                }
                if (options_.minimize) {
                    TdokuMinimize(options_.pencilmark, false, puzzle);
                }
            }

            // skip if the puzzle is the same as the pattern
            if (options_.clues_to_drop > 0) {
                if (strncmp(puzzle, pattern, size) == 0) {
                    continue;
                }
            }

            // set separator
            puzzle[size] = separator;
            // copy to output, including separator
            memcpy(output_puzzles + count * (size + 1), puzzle, size + 1);
            count++;

            // store the new one randomly
            which = util_.RandomUInt() % max_pattern;
            memcpy(pattern_list + which * size, puzzle, size);
        }

        return count;
    }
};

extern "C"
size_t TdokuGenerate(size_t num, bool pencilmark, uint64_t randomSeed, char* buffer, char separator){
    Options options = Options();
    options.pencilmark = pencilmark;
    options.random_seed = randomSeed;
    options.max_puzzles = num;
    Generator g(options);
    g.InitEmpty();
    return g.Generate(buffer, separator);
}
