CC = gcc
OPT_FLAGS = -O2 -march=native -Wall
NOOPT_FLAGS = -O0 -Wall

all: matmul matmul_noopt

# normal build, compiler optimizations on
matmul: matmul.c
	$(CC) $(OPT_FLAGS) -o matmul matmul.c

# same code, compiler optimizations off, so only our own loop changes count
matmul_noopt: matmul.c
	$(CC) $(NOOPT_FLAGS) -o matmul_noopt matmul.c

clean:
	rm -f matmul matmul_noopt results.txt
