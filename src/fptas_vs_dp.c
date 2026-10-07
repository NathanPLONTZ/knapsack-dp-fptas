/*
 * fptas_vs_dp.c -- FPTAS against the exact optimum (question 5).
 *
 * Draws one instance and solves it twice, with the exact dynamic program (OPT)
 * and with the FPTAS, so the loss caused by the interval reduction can be read
 * directly off the two profits.
 *
 * Usage: fptas_vs_dp [n] [Vmax] [eps] [seed]
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

  check_size(n);

  {
    int profits[n];
    int volumes[n];
    int pmax;
    StateList *exact_set;
    StateList *fptas_set;
    const Node *best;

    seed_rng(argc, argv, 4);
    fill_random(profits, n, 100);
    fill_random(volumes, n, 100);
    pmax = max_profit(profits, n);

    printf("Delta: %lf\n", fptas_delta(pmax, n, eps));

    exact_set = dp_solve(n, profits, volumes, Vmax);
    best = find_max_profit(exact_set);
    printf("\nOPT: %d\n", (best != NULL) ? best->data.p : 0);
    list_destroy(exact_set);

    fptas_set = fptas_solve(n, profits, volumes, Vmax, pmax, eps);
    print_result(fptas_set, "Max");
    list_destroy(fptas_set);
  }

  print_elapsed(start);
  return 0;
}
