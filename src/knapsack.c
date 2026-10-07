/*
 * knapsack.c -- Implementation of the shared state-space machinery.
 *
 * See include/knapsack.h for the contract of each function.
 */
#include "knapsack.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* State set lifecycle                                                */
/* ------------------------------------------------------------------ */

StateList *list_create(void) {
  StateList *list = malloc(sizeof(StateList));
  if (list == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    exit(EXIT_FAILURE);
  }
  list->size = 0;
  list->head = NULL;
  list->tail = NULL;
  return list;
}

void list_append(StateList *list, State value) {
  Node *node = malloc(sizeof(Node));
  if (node == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    exit(EXIT_FAILURE);
  }

  node->data = value;
  node->next = NULL;
  node->prev = list->tail;

  if (list->tail != NULL) {
    list->tail->next = node;
  } else {
    list->head = node;
  }

  list->tail = node;
  list->size++;
}

void list_remove(StateList *list, Node *node) {
  if (list == NULL || node == NULL) {
    return;
  }

  if (node->prev != NULL) {
    node->prev->next = node->next;
  } else {
    list->head = node->next;
  }

  if (node->next != NULL) {
    node->next->prev = node->prev;
  } else {
    list->tail = node->prev;
  }

  free(node);
  list->size--;
}

void list_destroy(StateList *list) {
  Node *current;

  if (list == NULL) {
    return;
  }

  current = list->head;
  while (current != NULL) {
    Node *doomed = current;
    current = current->next;
    free(doomed);
  }
  free(list);
}

/* ------------------------------------------------------------------ */
/* State set reductions                                               */
/* ------------------------------------------------------------------ */

void expand_states(const StateList *source, StateList *target, int item,
                   int profit, int volume, int Vmax) {
  const Node *current = source->head;

  while (current != NULL) {
    /* Carry the state over unchanged: the item is simply not taken. */
    list_append(target, current->data);

    /* Take the item too, whenever the knapsack can still hold it. */
    if (current->data.v + volume <= Vmax) {
      State extended = current->data;
      extended.p = current->data.p + profit;
      extended.v = current->data.v + volume;

      /* Record the item in the selection history. The bound keeps the write
         inside usedItems even if the caller skipped check_size(). */
      if (extended.itemCount < MAX_ITEMS) {
        extended.usedItems[extended.itemCount] = item;
        extended.itemCount = current->data.itemCount + 1;
      }

      list_append(target, extended);
    }

    current = current->next;
  }
}

void remove_exact_duplicates(StateList *list) {
  Node *current = list->head;

  while (current != NULL) {
    Node *check = current->next;

    while (check != NULL) {
      Node *after = check->next;

      if (current->data.p == check->data.p &&
          current->data.v == check->data.v) {
        list_remove(list, check);
      }

      check = after;
    }

    /* Read the successor only now: the inner loop may have removed the node
       that followed current when this iteration started. */
    current = current->next;
  }
}

void keep_min_volume_per_profit(StateList *list) {
  Node *current = list->head;

  while (current != NULL) {
    Node *resume = NULL;
    int removed_current = 0;
    Node *check = current->next;

    while (check != NULL) {
      Node *after = check->next;

      if (current->data.p == check->data.p) {
        if (current->data.v < check->data.v) {
          /* current dominates: drop the later state and keep scanning. */
          list_remove(list, check);
        } else {
          /* check is at least as good, so current goes. Capture the live
             successor before freeing, then resume from it. */
          resume = current->next;
          list_remove(list, current);
          removed_current = 1;
          break;
        }
      }

      check = after;
    }

    current = removed_current ? resume : current->next;
  }
}

double fptas_delta(int pmax, int n, double eps) {
  return (eps * pmax) / n;
}

/* Returns the minimum-volume state whose profit lies in [lower, upper), or NULL
   when the interval holds no state at all.
 *
 * The candidate starts out as NULL and the running minimum at Vmax + 1, so that
 * a state filling the knapsack exactly (v == Vmax) is still eligible and the
 * result is always a state of the interval. An earlier version seeded the
 * candidate with the list head and the minimum with Vmax, which returned a
 * state from outside the interval whenever no member had v < Vmax -- the caller
 * then compared against the wrong volume and deleted every state of the
 * interval. Minimal case: n=2, Vmax=8, items [p=10,v=11] and [p=6,v=8] made the
 * FPTAS answer 0 instead of the optimum 6.
 */
static Node *min_volume_in_range(const StateList *list, double lower,
                                 double upper, int Vmax) {
  Node *current = list->head;
  Node *best = NULL;
  int min = Vmax + 1;

  while (current != NULL) {
    if (current->data.p >= lower && current->data.p < upper) {
      if (current->data.v < min) {
        min = current->data.v;
        best = current;
      }
    }
    current = current->next;
  }

  return best;
}

void fptas_reduce(StateList *list, int n, int pmax, int Vmax, double eps) {
  const double limit = (double)pmax * n;
  const double delta = fptas_delta(pmax, n, eps);
  double lower = 0.0;

  /* A non-positive width would never advance the sweep. */
  if (delta <= 0.0) {
    return;
  }

  while (lower < limit) {
    Node *best = min_volume_in_range(list, lower, lower + delta, Vmax);
    Node *current = list->head;

    while (best != NULL && current != NULL) {
      Node *after = current->next;

      if (current->data.p >= lower && current->data.p < lower + delta) {
        if (current->data.v > best->data.v) {
          list_remove(list, current);
        } else if (current->data.v == best->data.v &&
                   current->data.p != best->data.p) {
          list_remove(list, current);
        }
      }

      current = after;
    }

    lower += delta;
  }
}

/* ------------------------------------------------------------------ */
/* Queries and reporting                                              */
/* ------------------------------------------------------------------ */

Node *find_max_profit(const StateList *list) {
  Node *current = list->head;
  Node *best = current;

  while (current != NULL) {
    if (current->data.p > best->data.p) {
      best = current;
    }
    current = current->next;
  }

  return best;
}

void print_state_set(const StateList *list) {
  const Node *current = list->head;

  if (current == NULL) {
    printf("\nFinal set (Xn): empty\n");
    return;
  }

  printf("\nFinal set (Xn): ");
  while (current != NULL) {
    printf("[%d,%d] ", current->data.p, current->data.v);
    current = current->next;
  }
  printf("\n");
}

void print_result(const StateList *list, const char *label) {
  const Node *best = find_max_profit(list);
  int i;

  if (best == NULL) {
    printf("\n%s: no state\n", label);
    return;
  }

  printf("\n%s: %d\n", label, best->data.p);
  printf("Items (index): ");
  for (i = 0; i < best->data.itemCount; i++) {
    printf("%d ", best->data.usedItems[i]);
  }
  printf("\n");
}

/* ------------------------------------------------------------------ */
/* Solvers                                                            */
/* ------------------------------------------------------------------ */

/* Builds the one-state set X0 = {[0,0]}. */
static StateList *initial_state_set(void) {
  StateList *list = list_create();
  State initial;

  initial.p = 0;
  initial.v = 0;
  initial.itemCount = 0;
  list_append(list, initial);

  return list;
}

StateList *dp_solve(int n, const int *profits, const int *volumes, int Vmax) {
  StateList *current = initial_state_set();
  int i;

  for (i = 1; i <= n; i++) {
    StateList *next = list_create();

    expand_states(current, next, i, profits[i - 1], volumes[i - 1], Vmax);

    /* Improvement of question 1: among equal profits keep the smallest volume. */
    keep_min_volume_per_profit(next);

    list_destroy(current);
    current = next;
  }

  return current;
}

StateList *fptas_solve(int n, const int *profits, const int *volumes, int Vmax,
                       int pmax, double eps) {
  StateList *current = initial_state_set();
  int i;

  for (i = 1; i <= n; i++) {
    StateList *next = list_create();

    expand_states(current, next, i, profits[i - 1], volumes[i - 1], Vmax);

    remove_exact_duplicates(next);
    fptas_reduce(next, n, pmax, Vmax, eps);

    list_destroy(current);
    current = next;
  }

  return current;
}

State greedy_by_ratio(int n, const int *profits, const int *volumes, int Vmax) {
  State result;
  int i = 0;

  result.p = 0;
  result.v = 0;
  result.itemCount = 0;

  while (result.v <= Vmax && i < n) {
    if (result.v + volumes[i] > Vmax) {
      i++;
      continue;
    }
    result.p += profits[i];
    result.v += volumes[i];
    if (result.itemCount < MAX_ITEMS) {
      result.usedItems[result.itemCount] = i;
      result.itemCount++;
    }
    i++;
  }

  return result;
}

/* ------------------------------------------------------------------ */
/* Instance helpers                                                   */
/* ------------------------------------------------------------------ */

int max_profit(const int *profits, int n) {
  int max;
  int i;

  if (n <= 0) {
    return 0;
  }

  max = profits[0];
  for (i = 1; i < n; i++) {
    if (profits[i] > max) {
      max = profits[i];
    }
  }
  return max;
}

void fill_random(int *arr, int count, int upper) {
  int i;
  for (i = 0; i < count; i++) {
    arr[i] = rand() % upper + 1;
  }
}

void compute_ratios(double *ratios, const int *profits, const int *volumes,
                    int n) {
  int i;
  for (i = 0; i < n; i++) {
    ratios[i] = (double)profits[i] / volumes[i];
  }
}

/* Swaps entry a with entry b across the three parallel arrays. */
static void swap_entries(double *ratios, int *volumes, int *profits, int a,
                         int b) {
  double ratio = ratios[a];
  int volume = volumes[a];
  int profit = profits[a];

  ratios[a] = ratios[b];
  volumes[a] = volumes[b];
  profits[a] = profits[b];

  ratios[b] = ratio;
  volumes[b] = volume;
  profits[b] = profit;
}

/* Lomuto partition on decreasing ratio, ties broken by decreasing volume. */
static int partition_by_ratio(double *ratios, int low, int high, int *volumes,
                              int *profits) {
  double pivot = ratios[high];
  int i = low - 1;
  int j;

  for (j = low; j < high; j++) {
    if (ratios[j] > pivot ||
        (ratios[j] == pivot && volumes[j] > volumes[high])) {
      i++;
      swap_entries(ratios, volumes, profits, i, j);
    }
  }

  swap_entries(ratios, volumes, profits, i + 1, high);
  return i + 1;
}

void quicksort_by_ratio(double *ratios, int low, int high, int *volumes,
                        int *profits) {
  if (low < high) {
    int pivot = partition_by_ratio(ratios, low, high, volumes, profits);

    quicksort_by_ratio(ratios, low, pivot - 1, volumes, profits);
    quicksort_by_ratio(ratios, pivot + 1, high, volumes, profits);
  }
}

void print_items(const int *profits, const int *volumes, int n) {
  int i;
  printf("          p/v\n");
  for (i = 0; i < n; i++) {
    printf("Item %d : [%d,%d]\n", i + 1, profits[i], volumes[i]);
  }
}

void print_elapsed(clock_t start) {
  double seconds = ((double)(clock() - start)) / CLOCKS_PER_SEC;
  printf("\nExecution time: %f seconds.\n", seconds);
}

void check_size(int n) {
  if (n < 1) {
    fprintf(stderr, "Item count must be at least 1 (got %d).\n", n);
    exit(EXIT_FAILURE);
  }
  if (n > MAX_ITEMS) {
    fprintf(stderr,
            "Item count %d exceeds MAX_ITEMS (%d): a state cannot store a "
            "longer selection history. Raise MAX_ITEMS in include/knapsack.h "
            "and rebuild.\n",
            n, MAX_ITEMS);
    exit(EXIT_FAILURE);
  }
}

/* ------------------------------------------------------------------ */
/* Command-line arguments                                             */
/* ------------------------------------------------------------------ */

int arg_int(int argc, char **argv, int index, int fallback) {
  return (index < argc) ? atoi(argv[index]) : fallback;
}

double arg_double(int argc, char **argv, int index, double fallback) {
  return (index < argc) ? atof(argv[index]) : fallback;
}

void seed_rng(int argc, char **argv, int index) {
  if (index < argc) {
    srand((unsigned)strtoul(argv[index], NULL, 10));
  } else {
    srand((unsigned)time(NULL));
  }
}
