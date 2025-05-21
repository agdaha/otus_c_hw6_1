all: explorer

explorer: main.c
	$(CC) $^ -o $@ -Wall -Wextra -Wpedantic -std=c11 `pkg-config --cflags --libs gtk+-3.0`

clean:
	rm -f explorer

.PHONY: all clean
