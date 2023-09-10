#include "../include/tdoku.h"
#include "util.h"

double MeanLogGuesses(char *puzzle, bool pencilmark, int solver, int num_evals) {
    Util util{};
    char solution[81];
    double sum_log_guesses = 0.0;
    int count = 0;
    for (int j = 0; j < num_evals; j++) {
        util.PermuteSudoku(puzzle, pencilmark);
        size_t guesses = 0;

        // it may not solve, because some solvers doesn't support pencilmark
        if(TdokuSolve(puzzle, solver, solution, &guesses)){
            sum_log_guesses += log((double) guesses + 1);
            count++;
        }
    }

    return count == 0 ? 0.0 : sum_log_guesses / count;
}

extern "C"
int TdokuRate(char *input, int solver, int num_evals){
    bool pencilmark = input[81] >= '.';
    
    double l = MeanLogGuesses(input, pencilmark, solver, num_evals);

    return (int)std::round(l/log(9)*1000);
}

extern "C"
bool TdokuSolve(const char *input, int solver, char *solution, size_t *num_guesses){
    bool pencilmark = input[81] >= '.';
    switch (solver)
    {
        case 0:
            return TdokuSolverDpllTriadSimd(input, 1, 0, solution, num_guesses) > 0;
        case 1:
            return !pencilmark && TdokuSolverDpllTriadScc(input, 1, 0, solution, num_guesses) > 0;
        case 2:
            return TdokuSolverBasic(input, 1, 0, solution, num_guesses) > 0;
    }

    return false;
}
