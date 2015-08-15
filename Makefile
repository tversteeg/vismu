CC=gcc
RM=rm -f
FLAGS=-g -Wall -Wextra -DCC_USE_ALL
LDFLAGS=-L/usr/lib -L/usr/local/lib -L. -lccore -lasound -lfftw3 -lm -lpthread -lX11 -lXrandr -lXinerama -lXi -lGL -lGLU -lGLEW

SRCS=main.c
OBJS=$(subst .c,.o,$(SRCS))

all: vismu

vismu: $(OBJS)
	$(CC) -o vismu $(OBJS) $(LDFLAGS) 

$(OBJS): $(SRCS)
	$(CC) -o $(OBJS) -c $(FLAGS) $(SRCS)

clean:
	$(RM) $(OBJS)

dist-clean: clean
	$(RM) vismu
