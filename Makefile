CC=gcc
RM=rm -f
LDFLAGS=-g -Wall -Wextra -lasound -lfftw3 -lm -lpthread

SRCS=main.c
OBJS=$(subst .c,.o,$(SRCS))

all: vismu

vismu: $(OBJS)
	$(CC) $(LDFLAGS) -o vismu $(OBJS)

main.o: main.c

clean:
	$(RM) $(OBJS)

dist-clean: clean
	$(RM) vismu
