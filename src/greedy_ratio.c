/*
 * greedy_ratio.c -- Greedy heuristic by decreasing p/v (question 2).
 *
 * Sorts the items by decreasing profit-to-volume ratio with quicksort, then
 * walks the sorted list taking every item that still fits.
 *
 * Note: the sort permutes the item arrays in place, so the indices printed
 * below are positions in the sorted order, not in the generated instance.
 *
 * Usage: greedy_ratio [n] [Vmax] [seed]
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

  check_size(n);

  {
    int profits[n];
    int volumes[n];
    double ratios[n];
    State result;
    int i;

    seed_rng(argc, argv, 3);
    fill_random(profits, n, 100);
    fill_random(volumes, n, 100);

    compute_ratios(ratios, profits, volumes, n);
    quicksort_by_ratio(ratios, 0, n - 1, volumes, profits);

    result = greedy_by_ratio(n, profits, volumes, Vmax);

    printf("Max: %d\n", result.p);
    printf("Items (rank after sorting): ");
    for (i = 0; i < result.itemCount; i++) {
      printf("%d ", result.usedItems[i] + 1);
    }
    printf("\n");
  }

  print_elapsed(start);
  return 0;
}
