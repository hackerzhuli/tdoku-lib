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
    int num_puzzles_in_pool = 500;
    bool minimize = true;
    bool pencilmark = false;
};

struct Generator {
    Options options_;
    Util util_{};
    vector<pair<double, string>> pattern_heap{};
    set<string> pattern_set{};

    explicit Generator(const Options &options) : options_(options) {}

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
        double loss = numeric_limits<double>::max();
        for (int i = 0; i < options_.num_puzzles_in_pool; i++) {
            const string &initial = options_.pencilmark ? kInitPencilmark : kInitVanilla;
            pattern_heap.emplace_back(make_pair(loss, initial));
        }
        make_heap(pattern_heap.begin(), pattern_heap.end());
    }

    bool HasUniqueSolution(const char *puzzle) {
        char solution[81];
        size_t guesses = 0;
        return TdokuSolverDpllTriadSimd(puzzle, 2, 0, solution, &guesses) == 1;
    }

    double MeanLogGuesses(char *puzzle) {
        char solution[81];
        double sum_log_guesses = 0.0;
        for (int j = 0; j < options_.num_evals; j++) {
            util_.PermuteSudoku(puzzle, options_.pencilmark);
            size_t guesses = 0;
            TdokuSolverDpllTriadSimd(puzzle, 1, 0, solution, &guesses);
            sum_log_guesses += log((double) guesses + 1);
        }
        return options_.num_evals == 0 ? 0.0 : sum_log_guesses / options_.num_evals;
    }

    int NumClues(const char *puzzle) {
        int num_clues = 0;
        if (options_.pencilmark) {
            // for pencilmark a clue is an elimination
            for (int i = 0; i < 729; i++) {
                if (puzzle[i] == '.') num_clues++;
            }
        } else {
            // for vanilla a clue is a cell placement
            for (int i = 0; i < 81; i++) {
                if (puzzle[i] != '.') num_clues++;
            }
        }
        return num_clues;
    }

    tuple<int, double, double> Evaluate(const char *puzzle) {
        char eval_puzzle[729];
        strncpy(eval_puzzle, puzzle, 729);

        int num_clues = NumClues(eval_puzzle);
        double mean_log_guesses = MeanLogGuesses(eval_puzzle);

        double loss;
        if (HasUniqueSolution(eval_puzzle)) {
            loss = num_clues * options_.clue_weight
                   - exp(mean_log_guesses * options_.guess_weight)
                   + util_.RandomDouble() * options_.random_weight;
        } else {
            loss = numeric_limits<double>::max();
        }
        return make_tuple(num_clues, exp(mean_log_guesses), loss);
    }

    void Load(const string &pattern_filename) {
        ifstream file;
        file.open(pattern_filename);
        if (file.fail()) {
            cout << "Error opening " << pattern_filename << endl;
            exit(1);
        }
        string line;
        char buffer[729];
        int num_loaded = 0;
        while (getline(file, line)) {
            if (line.length() == 0 || line[0] == '#') {
                continue;
            }
            line = line.substr(0, options_.pencilmark ? 729 : 81);
            strncpy(buffer, line.c_str(), 729);
            double loss = get<2>(Evaluate(buffer));
            pattern_heap.emplace_back(make_pair(loss, line));
            pattern_set.insert(line);
            num_loaded++;
        }
        make_heap(pattern_heap.begin(), pattern_heap.end());
    }

    size_t Generate(char* output_puzzles, char separator) {
        char puzzle[729];
        uint32_t count = 0;

        size_t size = options_.pencilmark ? 729 : 81;
        for (uint64_t i = 0; i < options_.max_puzzles; i++) {
            // draw a puzzle or pattern from the pool
            size_t which = util_.RandomUInt() % pattern_heap.size();
            string &pattern = pattern_heap[which].second;
            memcpy(puzzle, pattern.c_str(), size);
            if (size == 81) puzzle[81] = '\0';

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

            // evaluate difficulty via guess counting
            auto eval_stats = Evaluate(puzzle);
            int num_clues = get<0>(eval_stats);
            double geo_mean_guesses = get<1>(eval_stats);
            double loss = get<2>(eval_stats);

            // skip if the puzzle is a duplicate of one still in the pool
            if (options_.clues_to_drop > 0) {
                if (strncmp(puzzle, pattern.c_str(), 729) == 0) {
                    continue;
                }
                if (pattern_set.find(puzzle) != pattern_set.end()) {
                    continue;
                }
            }

            if (loss > pattern_heap.front().first) {
                continue;
            }

            // copy to output and append an separator
            memcpy(output_puzzles + count * (size + 1), puzzle, size);
            output_puzzles[count * (size + 1) + size] = separator;
            count++;

            // ooadd the generated puzzle to the pl and kick out the one with highest loss
            pattern_set.insert(puzzle);
            pattern_heap.emplace_back(make_pair(loss, puzzle));
            push_heap(pattern_heap.begin(), pattern_heap.end());
            pop_heap(pattern_heap.begin(), pattern_heap.end());
            if (pattern_set.find(pattern_heap.back().second) != pattern_set.end()) {
                pattern_set.erase(pattern_heap.back().second);
            }
            pattern_heap.pop_back();
        }

        return count;
    }
};

extern "C"
size_t TdokuGenerate(size_t num, bool pencilmark, char* buffer, char separator){
    Options options = Options();
    options.pencilmark = pencilmark;
    Generator g(options);
    return g.Generate(buffer, separator);
}
