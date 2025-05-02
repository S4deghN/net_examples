CFLAGS := -Wall -O0 -g

programs = $(patsubst %.c,bin/%,$(wildcard *.c))

all: $(programs)

debug: CFLAGS += -g
debug: all

bin/%: %.c | bin
	$(CC) $(CFLAGS) $< -o $@

bin:
	mkdir -p bin

clean:
	rm bin/*
