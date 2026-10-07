/*
 * fptas.c -- FPTAS derived from the dynamic program (question 3).
 *
 * Same state-space expansion as dp.c, but the reduction step keeps one state
 * of minimum volume per profit interval of width delta = eps * pmax / n
 * instead of one per exact profit value.
 *
 * Usage: fptas [n] [Vmax] [eps] [seed]
 *        n     number of items                 (default 10)
 *        Vmax  knapsack capacity               (default 100)
 *        eps   interval parameter              (default 2)
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
  double eps = arg_double(argc, argv, 3, 2.0);
  StateList *final_set;

  check_size(n);

  {
    int profits[n];
    int volumes[n];
    int pmax;

    seed_rng(argc, argv, 4);
    fill_random(profits, n, 100);
    fill_random(volumes, n, 100);
    pmax = max_profit(profits, n);

    printf("Delta: %lf\n", fptas_delta(pmax, n, eps));

    final_set = fptas_solve(n, profits, volumes, Vmax, pmax, eps);
  }

  print_result(final_set, "Max");
  list_destroy(final_set);

  print_elapsed(start);
  return 0;
}
