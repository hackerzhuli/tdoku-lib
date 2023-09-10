#ifndef TDOKU_H
#define TDOKU_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#endif

struct GenerateOut9x9{
    uint32_t num_clues;
    float mean_guesses;
    float loss;
    char data[81];
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Solves a Sudoku or Pencilmark Sudoku puzzle.
 * @param input
 *      An 81 character string representing a standard sudoku puzzle, or a 729 character string
 *      representing a pencilmark sudoku puzzle. For standard sudoku for row 0..8 and col 0..8,
 *      the value at input[row * 9 + col] should be a digit '1'..'9' for a given clue or '.' for
 *      an empty cell. For pencilmark sudoku for row 0..8, col 0..8, and digit 1..9, the value
 *      at input[row * 81 + col * 9 + digit - 1] should be '.' if the digit is eliminated or else
 *      (char)('0' + digit). No input validation is done. Do not pass '0' instead of '.' for
 *      unknown cells or eliminated digits. The solver decides if it is dealing with a standard
 *      sudoku or pencilmark sudoku by checking whether input[81] is >= '.' (vs. newline or null).
 * @param limit
 *       The maximum number of solutions to find before returning.
 * @param configuration
 *       Solver-specific configuration. Unused for tdoku.
 * @param solution
 *       Pointer to an 81 character array to receive the solution. Tdoku will only return
 *       a solution if it was given a limit of 1. Otherwise it's assumed we're just interested
 *       in solution counts (e.g., 0, 1, 2+).
 * @param solver
 *       0, for the SIMD solver, 1 for scc solver, 2 for basic solver, note that basic solver does not support pencilmark, always return false
 * @param num_guesses
 *       Out parameter to receive the number of guesses performed during search.
 * @return
 *       whether a solution is found
 */
bool TdokuSolve(const char *input, int solver, char *solution, size_t *num_guesses);

/**
 * Solves a Sudoku or Pencilmark Sudoku puzzle.
 * @param input
 *       same as TdokuSolve
 * @param limit
 *       The maximum number of solutions to find before returning.
 * @param configuration
 *       Solver-specific configuration. Unused for tdoku.
 * @param solution
 *       Pointer to an 81 character array to receive the solution. Tdoku will only return
 *       a solution if it was given a limit of 1. Otherwise it's assumed we're just interested
 *       in solution counts (e.g., 0, 1, 2+).
 * @param num_guesses
 *       Out parameter to receive the number of guesses performed during search.
 * @return
 *       The number of solutions found up to the given limit.
 */
size_t TdokuSolverDpllTriadSimd(const char *input,
                                size_t limit,
                                uint32_t configuration,
                                char *solution,
                                size_t *num_guesses);

/**
 * Solves a Sudoku or Pencilmark Sudoku puzzle.
 * @param input
 *      same as TdokuSolve
 * @param limit
 *       The maximum number of solutions to find before returning.
 * @param configuration
 *       Solver-specific configuration. Unused for tdoku.
 * @param solution
 *       Pointer to an 81 character array to receive the solution. Tdoku will only return
 *       a solution if it was given a limit of 1. Otherwise it's assumed we're just interested
 *       in solution counts (e.g., 0, 1, 2+).
 * @param num_guesses
 *       Out parameter to receive the number of guesses performed during search.
 * @return
 *       The number of solutions found up to the given limit.
 */
size_t TdokuSolverDpllTriadScc(const char *input, size_t limit, uint32_t configuration,
                               char *solution, size_t *num_guesses);


/**
 * Solves a Sudoku puzzle.
 * @param input
 *      same as TdokuSolve
 * @param limit
 *       The maximum number of solutions to find before returning.
 * @param configuration
 *       Solver-specific configuration. Unused for tdoku.
 * @param solution
 *       Pointer to an 81 character array to receive the solution. Tdoku will only return
 *       a solution if it was given a limit of 1. Otherwise it's assumed we're just interested
 *       in solution counts (e.g., 0, 1, 2+).
 * @param num_guesses
 *       Out parameter to receive the number of guesses performed during search.
 * @return
 *       The number of solutions found up to the given limit.
 */
size_t TdokuSolverBasic(const char *input, size_t limit, uint32_t configuration,
                        char *solution, size_t *num_guesses);

/**
 * Enumerates all solutions to a given Sudoku or Pencilmark Sudoku puzzle.
 * @param puzzle
 *      The input puzzle, as described above.
 * @param limit
 *      The maximum number of solutions to return
 * @param callback
 *      A callback for notifying the caller of each solution
 * @param callback_arg
 *      An optional callback argument that will be returned as the second callback argument to
 *      facilitate thunks for capturing closures.
 * @return
 */
size_t TdokuEnumerate(const char *puzzle,
                      size_t limit,
                      void (*callback)(const char *, void *),
                      void *callback_arg);

/**
 * Given a partially constrained puzzle adds random clues until the solution is unique. This
 * procedure is fast, but biased in the sense that different puzzles may arise with widely
 * varying probabilities and it makes no effort to adjust for these differences or to estimate
 * these probabilities. It also does not guarantee that the resulting puzzle is minimal.
 * @param pencilmark
 *       A boolean indicating if the second argument is a pencilmark sudoku (vs. a vanilla one)
 * @param puzzle
 *       An in/out buffer of 81 or 729 characters containing the input partial puzzle and
 *       output re-constrained puzzle.
 * @return
 *       A boolean indicating success or failure.
 */
bool TdokuConstrain(bool pencilmark, char *puzzle);


/**
 * Minimizes a vanilla or pencilmark puzzle by testing removal of all clues in random order,
 * restoring any clue that's required to keep the solution unique.
 * @param pencilmark
 *       A boolean indicating whether to minimize a pencilmark sudoku (vs. a vanilla one)
 * @param monotonic
 *       A boolean indicating the minimizer should return true only if we have a minimal
 *       puzzle after the first restored clue.
 * @param puzzle
 *       An 81 or 729 character puzzle to minimize (for vanilla vs. pencilmark)
 */
bool TdokuMinimize(bool pencilmark, bool monotonic, char *puzzle);

/**
 * Generate classic sudoku puzzles 9x9
 * @param num
 *       number of puzzles to generate
 * @param pencilmark
 *       A boolean indicating whether to minimize a pencilmark sudoku (vs. a vanilla one)
 * @param buffer
 *       The output puzzles
 * @param separator
 *       A seperator is appended at the end of each puzzle
 * @return
 *       The number of puzzles generated, up to num, note that no puzzle is guaranteed to generate
 */
size_t TdokuGenerate(size_t num, bool pencilmark, char* buffer, char separator);

/**
 * returns an rating of the puzzle
 * 
 * rating formula:
 * 
 * ln(n_guess + 1) / ln(9) * 1000
 * 
 * ln() is natural log
 * n_guess is the number of guesses the solver performed
 * 
 * an average rating is obtained by solving the permutations of the same puzzle num_evals times
 * 
 * Note: this method is slower, the solver is called num_evals times under the hood
 * @param input
 *      same as TdokuSolve
 * @param solver
 *       0, for the SIMD solver, 1 for scc solver, 2 for basic solver
 * @param num_evals
 *      the number of times the solver runs to obtain an average rating
 * @return
 *       The rating, if puzzle is not supported by solver, 100 is returned
 */
int TdokuRate(const char *input, int solver, int num_evals);

#ifdef __cplusplus
}
#endif

static inline bool Minimize(bool pencilmark, bool monotonic, char *puzzle) {
    return TdokuMinimize(pencilmark, monotonic, puzzle);
}

static inline bool Constrain(bool pencilmark, char *puzzle) {
    return TdokuConstrain(pencilmark, puzzle);
}

static inline size_t Enumerate(const char *puzzle, size_t limit,
                               void (*callback)(const char *, void *),
                               void *callback_arg) {
    return TdokuEnumerate(puzzle, limit, callback, callback_arg);
}

static inline size_t SolveSudoku(const char *input, size_t limit, uint32_t configuration,
                                 char *solution, size_t *num_guesses) {
    return TdokuSolverDpllTriadSimd(input, limit, configuration, solution, num_guesses);
}
#endif //TDOKU_H
