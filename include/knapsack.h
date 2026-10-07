/*
 * knapsack.h -- Shared state-space machinery for the 0/1 knapsack mini-project.
 *
 * Every executable in this project explores the same state space: a state is a
 * reachable (profit, volume) pair, and the set X_i holds every state reachable
 * using only the first i items. The algorithms differ solely in the reduction
 * applied to X_i after each expansion step:
 *
 *   - exact dynamic programming keeps, per profit value, the minimum volume;
 *   - the FPTAS keeps, per profit interval of width delta, the minimum volume.
 *
 * The greedy p/v heuristic does not use the state space at all and is provided
 * here only because two executables share it.
 */
#ifndef KNAPSACK_H
#define KNAPSACK_H

#include <time.h> /* clock_t, used by print_elapsed */

/* Upper bound on the number of items whose selection history a state can hold.
   A state stores its history inline, so this also caps the problem size. */
#define MAX_ITEMS 1000

/* One state of the state space: a reachable (profit, volume) pair together with
   the items that produced it. Item indices are 1-based, matching the report. */
typedef struct State {
  int p;                    /* accumulated profit */
  int v;                    /* accumulated volume */
  int usedItems[MAX_ITEMS]; /* 1-based indices of the selected items */
  int itemCount;            /* number of valid entries in usedItems */
} State;

/* Node of the doubly linked list holding a state set. */
typedef struct Node {
  State data;
  struct Node *next;
  struct Node *prev;
} Node;

/* A state set X_i. `size` is kept in sync by every insertion and removal. */
typedef struct StateList {
  int size;
  Node *head;
  Node *tail;
} StateList;

/* ------------------------------------------------------------------ */
/* State set lifecycle                                                */
/* ------------------------------------------------------------------ */

/* Allocates an empty state set. Exits on allocation failure. */
StateList *list_create(void);

/* Appends a copy of `value` to the end of `list`. Exits on allocation failure. */
void list_append(StateList *list, State value);

/* Unlinks and frees `node`, keeping `list->size` correct. No-op on NULL. */
void list_remove(StateList *list, Node *node);

/* Frees every node and the list header itself. */
void list_destroy(StateList *list);

/* ------------------------------------------------------------------ */
/* State set reductions                                               */
/* ------------------------------------------------------------------ */

/* Expands `source` into `target` by considering item `item` (1-based): every
   state is carried over, and the state extended with the item is added when it
   still fits within `Vmax`. */
void expand_states(const StateList *source, StateList *target, int item,
                   int profit, int volume, int Vmax);

/* Drops states that duplicate an earlier state exactly (same profit and same
   volume). */
void remove_exact_duplicates(StateList *list);

/* Dominance rule of the improved dynamic program: among states sharing a
   profit, keeps only one of minimum volume. */
void keep_min_volume_per_profit(StateList *list);

/* FPTAS reduction: slices the profit axis into intervals of width
   delta = eps * pmax / n and keeps, per interval, the state of minimum volume. */
void fptas_reduce(StateList *list, int n, int pmax, int Vmax, double eps);

/* Width of one FPTAS profit interval. */
double fptas_delta(int pmax, int n, double eps);

/* ------------------------------------------------------------------ */
/* Queries and reporting                                              */
/* ------------------------------------------------------------------ */

/* Returns the first state of maximal profit, or NULL if the set is empty. */
Node *find_max_profit(const StateList *list);

/* Prints every (profit, volume) pair of the set. */
void print_state_set(const StateList *list);

/* Prints the best profit of the set under `label`, then the items achieving it. */
void print_result(const StateList *list, const char *label);

/* ------------------------------------------------------------------ */
/* Solvers                                                            */
/* ------------------------------------------------------------------ */

/* Exact dynamic program. Returns the final state set X_n; the caller owns it. */
StateList *dp_solve(int n, const int *profits, const int *volumes, int Vmax);

/* FPTAS. Returns the final reduced state set X_n; the caller owns it. */
StateList *fptas_solve(int n, const int *profits, const int *volumes, int Vmax,
                       int pmax, double eps);

/* Greedy heuristic over items already sorted by decreasing p/v: takes each item
   in turn whenever it still fits. Item indices in the result are positions in
   the *sorted* arrays, not in the original instance. */
State greedy_by_ratio(int n, const int *profits, const int *volumes, int Vmax);

/* ------------------------------------------------------------------ */
/* Instance helpers                                                   */
/* ------------------------------------------------------------------ */

/* Largest profit of the instance. Returns 0 when n <= 0. */
int max_profit(const int *profits, int n);

/* Fills `arr` with `count` values drawn uniformly from [1, upper]. */
void fill_random(int *arr, int count, int upper);

/* Computes ratios[i] = profits[i] / volumes[i]. */
void compute_ratios(double *ratios, const int *profits, const int *volumes,
                    int n);

/* Sorts `ratios` in decreasing order, permuting `volumes` and `profits` along
   with it. Ties are broken by decreasing volume. */
void quicksort_by_ratio(double *ratios, int low, int high, int *volumes,
                        int *profits);

/* Prints the instance as one "profit/volume" line per item. */
void print_items(const int *profits, const int *volumes, int n);

/* Prints elapsed CPU time since `start` in the project's usual format. */
void print_elapsed(clock_t start);

/* Rejects sizes the inline selection history cannot represent. Exits on
   failure. */
void check_size(int n);

/* ------------------------------------------------------------------ */
/* Command-line arguments                                             */
/* ------------------------------------------------------------------ */

/* Returns argv[index] parsed as an int, or `fallback` when absent. */
int arg_int(int argc, char **argv, int index, int fallback);

/* Returns argv[index] parsed as a double, or `fallback` when absent. */
double arg_double(int argc, char **argv, int index, double fallback);

/* Seeds the generator from argv[index] when present, otherwise from the clock.
   An explicit seed makes a run reproducible. */
void seed_rng(int argc, char **argv, int index);

#endif /* KNAPSACK_H */
