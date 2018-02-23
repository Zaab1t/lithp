CC ?= gcc
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic

TARGET_EXEC ?= lithp
SRC_DIR ?= ./src
INC_DIR ?= ./include

SRCS := $(shell find $(SRC_DIR) -name *.c)
DEPS := $(shell find $(INC_DIR) -name *.c)

build:
	$(CC) $(CFLAGS) $(SRCS) -O0 -g -I $(INC_DIR) $(DEPS) -ledit -lm -o $(TARGET_EXEC)
