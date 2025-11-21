CC = gcc
CFLAGS = -std=c99 \
         -D_POSIX_C_SOURCE=200809L \
         -D_XOPEN_SOURCE=700 \
         -Wall -Wextra -Werror \
         -Wno-unused-parameter \
         -fno-asm \
         -g

OBJS = main.o prompt.o parser.o functs.o pipes.o jobs.o

myshell: $(OBJS)
	$(CC) $(CFLAGS) -o shell.out $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o shell.out
