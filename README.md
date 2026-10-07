# Knapsack: Dynamic Programming, Greedy Heuristic and FPTAS

A study in **algorithmics and complexity**: three ways to attack the 0/1 knapsack
problem, implemented in C99 over a single shared state-space engine, and
measured against each other on running time and solution quality.

The point of the exercise is not to ship a knapsack solver — it is to measure
what an exact method, a fast heuristic, and a tunable approximation scheme
actually cost and actually deliver on the same instances.

---

## Table of contents

- [Problem](#problem)
- [The three algorithms](#the-three-algorithms)
- [Complexity](#complexity)
- [Data](#data)
- [Repository layout](#repository-layout)
- [Building and running](#building-and-running)
- [Results](#results)
- [Verification](#verification)
- [Known issues and limitations](#known-issues-and-limitations)
- [Origin of this repository](#origin-of-this-repository)

---

## Problem

Each item `i` (for `i = 1 … N`) carries a profit `pᵢ` and a volume `vᵢ`. The
knapsack holds a total volume of `Vmax`. Select a subset of the items that
**maximises total profit** while keeping total volume within `Vmax`.

This repository implements three methods for it — exact dynamic programming
over the state space, a greedy heuristic, and an FPTAS derived from the
dynamic program — and compares their running time and their answers on the same
randomly generated instances.

---

## The three algorithms

All three share one representation. A **state** is a reachable
`(profit, volume)` pair together with the items that produced it, and `Xᵢ` is
the set of states reachable using only the first `i` items:

```
X₀ = { [0,0] }
for i = 1 … N:
    Xᵢ = ∅
    for each state [p,v] in Xᵢ₋₁:
        add [p,v]                                  # item i not taken
        add [p+pᵢ, v+vᵢ]  if v+vᵢ ≤ Vmax           # item i taken
    reduce(Xᵢ)
return max { p : [p,v] ∈ X_N }
```

The algorithms differ **only** in `reduce`. That is why they share
[`src/knapsack.c`](src/knapsack.c) and differ by a few lines each.

### 1. Exact dynamic programming — `reduce` = exact dominance

Without reduction, `|Xᵢ|` doubles at every step: 2^N states. The improvement is
a **dominance rule**: if two states have the same profit, only the one with the
smaller volume can ever lead anywhere, so the other is dropped.

```
for each pair [p,v], [p',v'] in Xᵢ with p = p':
    keep the one with the smaller volume
```

Since every profit lies in `0 … N·pmax`, the set now holds at most
`N·pmax + 1` states instead of `2^N`. This returns the exact optimum.

> A symmetric rule works too — among states of equal **volume**, keep the
> maximum profit. This implementation uses the profit-keyed version.

### 2. Greedy heuristic — decreasing `pᵢ/vᵢ`

Sort the items by decreasing profit-to-volume ratio (quicksort), then walk the
sorted list and take every item that still fits. Fast, and does not touch the
state space at all.

It carries **no quality guarantee**: a high-ratio item taken early can block a
larger one that no longer fits. How it behaves in practice on random instances
is measured in [Results](#results).

### 3. FPTAS — `reduce` = interval dominance

Relax the exact dominance into an approximate one. Instead of keeping one state
per exact profit value, slice the profit axis into intervals of width

```
δ = ε · pmax / N
```

and keep, **per interval**, only the state of minimum volume. A larger `ε`
means wider intervals, so fewer surviving states, so a faster run and a coarser
answer. `ε` is the accuracy knob, and it is a command-line parameter here.

---

## Complexity

| Algorithm | States per iteration | Total (as designed) |
|---|---|---|
| DP, no reduction | `2ⁱ` | `O(2^N)` |
| DP + exact dominance | `≤ N·pmax + 1` | `O(N² · pmax)` |
| Greedy `p/v` | — | `O(N log N)` average, `O(N²)` worst |
| FPTAS | `≤ N·pmax/δ = N²/ε` | `O(N³ / ε)` |

The DP is **pseudo-polynomial**: `pmax` is a numeric value, not an input length,
so the bound is exponential in the number of bits of the profits. The FPTAS
removes `pmax` from the bound entirely and replaces it with the accuracy knob
`1/ε` — which is the whole reason the scheme exists.

> **Implementation caveat.** The table above is the complexity of the algorithms
> as designed, and assumes the reduction step is `O(1)` per state (e.g. a table
> indexed by profit). This implementation stores states in a **doubly linked
> list** and reduces by pairwise scanning, so the realised cost is higher —
> roughly `O(N · (N·pmax)²)` for the DP and `O(N⁵/ε²)` for the FPTAS. This gap,
> not the theoretical bound, is what the timings below actually measure.

---

## Data

**There are no data files in this repository, and none are needed.**

Instances are generated at run time: profits `pᵢ` and volumes `vᵢ` are drawn
uniformly from `[1, 100]`, independently of each other. `N` and `Vmax` are
supplied on the command line.

Every program accepts an optional trailing **seed**. Omit it and the generator
is seeded from the clock (a fresh instance per run); pass one and the run is
exactly reproducible:

```console
$ ./build/dp 10 100 42

Max: 285
Items (index): 1 5 6 8

Execution time: 0.000000 seconds.
```

Because instances are drawn rather than stored, nothing heavy is ever committed.
`data/`, `results/` and the usual dump formats are listed in
[`.gitignore`](.gitignore) so that local measurement output stays local.

---

## Repository layout

```
.
├── include/
│   └── knapsack.h        Shared API: state, state set, reductions, solvers
├── src/
│   ├── knapsack.c        The single implementation of everything shared
│   ├── dp.c              Exact dynamic programming
│   ├── fptas.c           FPTAS alone
│   ├── greedy_ratio.c    Greedy p/v heuristic alone
│   ├── greedy_vs_dp.c    Greedy against the exact optimum
│   └── fptas_vs_dp.c     FPTAS against the exact optimum
├── Makefile
├── requirements.txt      States that there are no Python dependencies
└── .gitignore
```

Each `src/*.c` other than `knapsack.c` is a thin `main`: it reads its
parameters, draws an instance, calls a solver and prints. All the algorithmic
substance lives in `knapsack.c`.

---

## Building and running

**Requirements:** a C99 compiler (developed with MinGW gcc 6.3.0) and
optionally `make`. No third-party libraries — only `<stdio.h>`, `<stdlib.h>`
and `<time.h>`.

```console
$ make            # builds the five executables into build/
$ make run        # builds, then runs each one once with its defaults
$ make clean
```

Without `make`, each program is one command — compile a `main` together with the
shared module:

```console
$ gcc -std=c99 -O2 -Wall -Wextra -Iinclude src/dp.c src/knapsack.c -o dp
```

On Windows with MinGW, the `make` binary is usually called `mingw32-make`.

### Programs and parameters

Every parameter is optional and falls back to the default shown.

| Command | Parameters (defaults) | What it prints |
|---|---|---|
| `./build/dp` | `n=10  Vmax=100  seed=clock` | exact optimum + chosen items |
| `./build/fptas` | `n=10  Vmax=100  eps=2  seed=clock` | `δ`, approximate profit + items |
| `./build/greedy_ratio` | `n=10  Vmax=100  seed=clock` | heuristic profit + items |
| `./build/greedy_vs_dp` | `n=100  Vmax=50  seed=clock` | `OPT` then heuristic profit |
| `./build/fptas_vs_dp` | `n=10  Vmax=100  eps=2  seed=clock` | `δ`, `OPT`, then FPTAS profit |

```console
$ ./build/fptas_vs_dp 10 100 2 7      # N=10, Vmax=100, ε=2, seed 7
Delta: 19.000000

OPT: 221

Max: 216
Items (index): 1 2 4 6 9 10

Execution time: 0.000000 seconds.
```

Sweeping a parameter needs no recompilation:

```console
$ for e in 0.1 0.5 1 2 10; do ./build/fptas_vs_dp 12 100 $e 7; done
```

> **Item indices.** For `dp` and `fptas`, the printed indices are 1-based
> positions in the generated instance. For `greedy_ratio` and `greedy_vs_dp`,
> the quicksort permutes the item arrays in place, so the printed indices are
> **ranks after sorting**, not positions in the generated instance. The labels
> say which is which.

---

## Results

The timings below are the measurements taken for the original coursework, on
randomly generated instances with `pᵢ, vᵢ ∈ [1,100]`, averaged over runs. They
are reproduced here as recorded.

- `inst` — effectively instantaneous
- `>1min` — did not finish within one minute

### Running time — exact DP with dominance

Rows are `N`, columns are `Vmax`.

| `N` \ `Vmax` | 10 | 100 | 1000 | 10000 |
|---|---|---|---|---|
| **10** | inst | inst | 2 s | 55 s |
| **100** | inst | 0.3 s | 42 s | >1min |
| **1000** | inst | 5 s | >1min | >1min |
| **10000** | inst | 20 s | >1min | >1min |

The exact method is usable only in a narrow corner. It degrades with **both**
`N` and `Vmax`, which is the pseudo-polynomial behaviour made visible: `Vmax`
is a numeric value, and growing it by a factor of 10 costs far more than one
extra bit of input should.

### Running time — greedy `p/v`

| `N` \ `Vmax` | 10 | 100 | 1000 | 10000 |
|---|---|---|---|---|
| **10** | inst | inst | inst | inst |
| **100** | inst | inst | inst | inst |
| **1000** | inst | inst | inst | inst |
| **10000** | inst | inst | inst | inst |

Instantaneous everywhere, as `O(N log N)` predicts. The cost is quality.

### Running time — FPTAS

Rows are `ε`, columns are `N`.

**`Vmax` = 10**

| `ε` \ `N` | 10 | 100 | 1000 | 10000 |
|---|---|---|---|---|
| **1** | inst | inst | >1min | >1min |
| **10** | inst | inst | >1min | >1min |
| **100** | inst | inst | 3 s | >1min |
| **1000** | inst | inst | inst | >1min |
| **10000** | inst | inst | inst | 4 s |

**`Vmax` = 100**

| `ε` \ `N` | 10 | 100 | 1000 | 10000 |
|---|---|---|---|---|
| **1** | inst | 15 s | >1min | >1min |
| **10** | inst | inst | >1min | >1min |
| **100** | inst | inst | 22 s | >1min |
| **1000** | inst | inst | inst | >1min |
| **10000** | inst | inst | inst | 8 s |

**`Vmax` = 1000**

| `ε` \ `N` | 10 | 100 | 1000 | 10000 |
|---|---|---|---|---|
| **1** | inst | 1 min 10 s | >1min | >1min |
| **10** | inst | inst | >1min | >1min |
| **100** | inst | inst | >1min | >1min |
| **1000** | inst | inst | inst | >1min |
| **10000** | inst | inst | inst | 30 s |

The diagonal is the whole story: **larger `ε` is faster**. A larger `ε` widens
`δ`, so fewer intervals survive and the state set stays small. That is the
accuracy-for-time trade-off the scheme exists to expose, here as a measurement.

The absolute numbers depend heavily on drawing `pᵢ, vᵢ` from `[1,100]`, because
`pmax` sets `δ` directly.

### Solution quality

- **Exact DP** returns the optimum, by construction.
- **Greedy `p/v`** landed above 70% of the optimum on every instance tested, and
  moved closer to the optimum as `N` grew. Dense random instances offer many
  near-equivalent items, so an early greedy choice is rarely punished; this is a
  property of the instances tested, not a promise about any instance.
- **FPTAS** quality tracks `ε`: the smaller `ε`, the closer to the optimum.

> **On the values of `ε`.** The original measurements sweep `ε` from 1 to 10000.
> `ε` is meant to live in `(0,1]`, where `ε = 0.05` reads as "within 5% of the
> optimum"; past 1 it is acting as a raw state-count knob and the answer degrades
> fast. At `ε = 10` on `N = 10`, `Vmax = 100`, the FPTAS returned **95** against
> an optimum of **221**. For results that mean something, use `ε ≤ 1`.

---

## Verification

The exact DP and the FPTAS were cross-checked against an **independent
reference implementation** — the textbook `O(N·Vmax)` tabular knapsack DP, which
shares no code with this one — over 3000 random instances
(`N ≤ 10`, `Vmax ∈ [5,64]`, `pᵢ,vᵢ ∈ [1,40]`, `ε = 0.5`):

| Check | Result |
|---|---|
| `dp_solve` equals the reference optimum | **3000 / 3000** |
| FPTAS ever returns more than the optimum | 0 |
| Worst observed `FPTAS / OPT` | **0.7436** |

The whole project compiles with **zero warnings** under
`-std=c99 -Wall -Wextra`.

**What is not verified:** outputs were deliberately *not* compared against the
pre-refactoring version — the instances are redrawn each run, output labels were
translated to English, and one genuine bug fix (below) changes results on
purpose. The timing tables above are the original recorded measurements and were
not re-measured on this code.

---

## Known issues and limitations

Honest list of what is wrong or missing.

**Fixed during the rewrite**

- **FPTAS interval selection returned a state from outside the interval.** The
  minimum-volume search seeded its candidate with the list head and its running
  minimum with `Vmax`, so when no state in the interval had volume `< Vmax` it
  returned the head — which need not belong to the interval. The caller then
  compared volumes against the wrong state and deleted *every* state of the
  interval. Minimal case: `N=2`, `Vmax=8`, items `[p=10,v=11]` and `[p=6,v=8]`
  made the FPTAS answer **0** instead of the optimum **6**. Any instance where a
  subset fills the knapsack exactly was affected. Measured over 2293 random
  instances, the old logic returned **less than half the optimum 164 times**,
  worst case **0**; after the fix, no instance falls below 74% of the optimum.
- **Use-after-free in both reduction passes.** Each cached the successor of the
  current node *before* an inner loop that could free precisely that node, then
  advanced onto the freed pointer. Both now read the successor only after the
  inner loop.
- **The state-set size counter was never decremented** on removal, so
  `list->size` drifted upwards and a result guard of `size > 1` was meaningless.
  Removal now goes through one function that keeps the count correct.
- **Null dereference when the best profit was 0.** The maximum search returned
  `NULL` when no state had positive profit, and the caller dereferenced it
  without checking. It now returns the first state of maximal profit and the
  printer handles the empty set.
- **Memory leaks at exit**: freeing the list header abandoned every node.
- **Out-of-bounds write** on the inline selection history for `N > MAX_ITEMS`.
  Writes are now bounds-checked, and `check_size()` rejects oversized `N` up
  front with an explanatory message rather than corrupting the heap.
- A dead branch (`v == v' && v > v'`, never true), an unused local in the FPTAS
  loop, an unused array in the greedy, and an assigned-but-unused state.

**Still open**

- **`MAX_ITEMS` caps the problem at 1000 items.** Each state stores its full
  selection history inline (`int usedItems[1000]`), making a state ~4 KB and a
  state set enormous. This is the main reason the timings degrade so sharply —
  the work is memory traffic, not arithmetic. Replacing the inline history with
  a back-pointer to the parent state and the item index would shrink a state to
  ~24 bytes and lift the cap. **Consequence for the tables above: the `N=10000`
  rows exceed `MAX_ITEMS` and were undefined behaviour when originally
  measured.** This build refuses such runs instead of corrupting memory, so
  those rows cannot be reproduced without raising `MAX_ITEMS` and rebuilding.
- **Greedy item indices are post-sort ranks.** The quicksort permutes the item
  arrays in place and the original item positions are not tracked, so the greedy
  cannot report which items of the *generated* instance it chose. Only the
  profit is trustworthy. Fixing this needs an index array carried through the
  sort.
- **No greedy-vs-FPTAS comparison.** The two approximate methods are each
  compared against the exact optimum, never directly against each other.
- **The reduction steps are quadratic**, as noted under
  [Complexity](#complexity). Indexing states by profit would bring the
  implementation in line with the complexity claimed for the algorithm.
- **`δ` is accumulated in floating point** (`lower += δ`) across up to `N²/ε`
  iterations, so interval boundaries drift. Multiplying an integer counter by
  `δ` would avoid the accumulation.
- **No automated test suite.** The verification above was run from a throwaway
  harness that is not part of this repository.

---

## Origin of this repository

University mini-project for an M1 Computer Science course in advanced
algorithmics and complexity (2023–2024), restructured and republished here.

The rewrite deduplicated roughly 200 lines that had been copy-pasted across
seven source files — the shared state and list machinery existed in six
identical copies, bugs included — into one module, translated all commentary to
English, removed dead code, and fixed the defects listed above. File names now
describe what each program does:

| Original | Now |
|---|---|
| `DP.c` | `src/dp.c` |
| `FPTAS.c` | `src/fptas.c` |
| `Tri_PI_VI.c` | `src/greedy_ratio.c` |
| `Tri_vs_OPT.c` | `src/greedy_vs_dp.c` |
| `FPTAS_vs_OPT.c` | `src/fptas_vs_dp.c` |
| `Tri_vs_FPTAS.c` | folded into `src/fptas_vs_dp.c` (see note) |
| `algoFPTAS.c`, `main.c` | removed (abandoned draft; empty file) |

`Tri_vs_FPTAS.c` contained no sorting code at all: it was a copy of
`FPTAS_vs_OPT.c` with `ε = 10`. Since `ε` is now a command-line parameter, that
program is `./build/fptas_vs_dp 10 100 10`, and the misleading file is gone.
