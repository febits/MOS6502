CC=gcc
CFLAGS=-Wall -Wno-unused-function -Wno-unused-variable -g

CINCLUDE=-I./include
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
BIN=6502

.PHONY: clean default
default: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(CINCLUDE)
	@rm -f $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(CINCLUDE)

clean:
	-rm -f $(BIN)
