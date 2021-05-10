CC=gcc
CFLAGS=-Wall -Werror -fsanitize=address

all:
	$(CC) $(CFLAGS) ww.c -o ww

clean:
	rm -rf ww
