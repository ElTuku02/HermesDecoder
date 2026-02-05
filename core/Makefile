# HermesDecoder - Makefile 
CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -Iinc
LDFLAGS :=

TARGET  := hermesdecoder
SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Compila cada .c -> .o
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

# Ejecuta leyendo una trama por stdin (útil para pruebas rápidas)
run: $(TARGET)
	./$(TARGET)
