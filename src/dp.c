/*
 * dp.c -- Exact dynamic programming over the state space (question 1).
 *
 * Builds X_1 .. X_n and, after each expansion, applies the dominance rule:
 * among states sharing a profit, only one of minimum volume is kept. The best
 * profit of X_n is the optimum.
 *
 * Usage: dp [n] [Vmax] [seed]
 *        n     number of items                 (default 10)
 *        Vmax  knapsack capacity               (default 100)
 *        seed  RNG seed; omit for a clock seed
 */
#include "knapsack.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
  clock_t start = clock();

  int n = arg_int(argc, argv, 1, 10);
  int Vmax = arg_int(argc, argv, 2, 100);
  StateList *final_set;

  check_size(n);

  {
    int profits[n];
    int volumes[n];

    seed_rng(argc, argv, 3);
    fill_random(profits, n, 100);
    fill_random(volumes, n, 100);

    final_set = dp_solve(n, profits, volumes, Vmax);
  }

  print_result(final_set, "Max");
  list_destroy(final_set);

  print_elapsed(start);
  return 0;
}
