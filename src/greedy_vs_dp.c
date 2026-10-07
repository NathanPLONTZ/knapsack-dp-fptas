/*
 * greedy_vs_dp.c -- Greedy p/v heuristic against the exact optimum (question 5).
 *
 * Draws one instance, sorts it by decreasing p/v, then solves it twice: once
 * with the exact dynamic program (OPT) and once with the greedy heuristic.
 * Sorting does not change the optimum, so both answers describe the same
 * instance and the printed ratio is meaningful.
 *
 * Usage: greedy_vs_dp [n] [Vmax] [seed]
 *        n     number of items                 (default 100)
 *        Vmax  knapsack capacity               (default 50)
 *        seed  RNG seed; omit for a clock seed
 */
#include "knapsack.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
  clock_t start = clock();

  int n = arg_int(argc, argv, 1, 100);
  int Vmax = arg_int(argc, argv, 2, 50);

  check_size(n);

  {
    int profits[n];
    int volumes[n];
    double ratios[n];
    StateList *final_set;
    const Node *best;
    State heuristic;
    int i;

    seed_rng(argc, argv, 3);
    fill_random(profits, n, 100);
    fill_random(volumes, n, 100);

    compute_ratios(ratios, profits, volumes, n);
    quicksort_by_ratio(ratios, 0, n - 1, volumes, profits);

    final_set = dp_solve(n, profits, volumes, Vmax);
    best = find_max_profit(final_set);
    printf("\nOPT: %d\n", (best != NULL) ? best->data.p : 0);
    list_destroy(final_set);

    heuristic = greedy_by_ratio(n, profits, volumes, Vmax);
    printf("Heuristic max: %d\n", heuristic.p);
    printf("Items (rank after sorting): ");
    for (i = 0; i < heuristic.itemCount; i++) {
      printf("%d ", heuristic.usedItems[i] + 1);
    }
    printf("\n");
  }

  print_elapsed(start);
  return 0;
}
