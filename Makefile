CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -Wshadow

# Run make as `DEBUG=1 make` to disable optimizations.
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS += -Og -g
else
	CFLAGS += -Ofast -flto
endif

SRCDIR := ./src
OBJDIR := ./obj
INCDIR := ./include

SRC := $(wildcard $(SRCDIR)/*.c)
OBJ := $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
LIB := -lm -ledit

EXEC := lithp

.PHONY: all
all: $(EXEC)

.PHONY: clean
clean:
	rm -f $(EXEC)
	rm -f $(OBJ)

.PHONY: format
format:
	clang-format -i -style=file $(SRC) src/lithp.h

$(EXEC): $(OBJ)
	$(CC) -I$(INCDIR) $(CFLAGS) $(OBJ) $(LIB) -o $(EXEC)

$(OBJDIR)/%.o: $(OBJDIR) $(SRCDIR)/%.c
	$(CC) -I$(INCDIR) $(CFLAGS) $^ $(LIB) -c -o $@

$(OBJDIR):
	mkdir -p $@
