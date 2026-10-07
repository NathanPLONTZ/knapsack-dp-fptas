# Build all five knapsack executables into build/.
#
#   make           compile everything
#   make run       compile, then run each program once with its defaults
#   make clean     remove build/

# A plain `=` rather than `?=`: make ships a default CC of `cc`, which `?=`
# would not override. A command-line `make CC=clang` still wins.
CC      = gcc
CFLAGS  = -std=c99 -O2 -Wall -Wextra -Iinclude
LDFLAGS =

BUILD   := build
SHARED  := src/knapsack.c
HEADERS := include/knapsack.h

PROGRAMS := dp fptas greedy_ratio greedy_vs_dp fptas_vs_dp
BINARIES := $(addprefix $(BUILD)/,$(PROGRAMS))

.PHONY: all run clean

all: $(BINARIES)

$(BUILD)/%: src/%.c $(SHARED) $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $< $(SHARED) -o $@ $(LDFLAGS)

$(BUILD):
	mkdir -p $(BUILD)

run: all
	@for p in $(PROGRAMS); do \
		echo "=== $$p"; \
		$(BUILD)/$$p; \
		echo; \
	done

clean:
	rm -rf $(BUILD)
