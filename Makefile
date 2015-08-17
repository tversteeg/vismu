CC=gcc
RM=rm -f
CFLAGS=-g -Wall -Wextra -DCC_USE_ALL -DVISMU_DEBUG
LDFLAGS=-g -L/usr/lib -L/usr/local/lib -L. -lccore -lasound -lfftw3 -lm -lpthread -lX11 -lXrandr -lXinerama -lXi -lGL -lGLU -lGLEW

SRCS=main.c shader.c utils.c
OBJS=$(subst .c,.o,$(SRCS))

all: vismu

vismu: $(OBJS)
	$(CC) -o vismu $(OBJS) $(LDFLAGS) 

%.o: %.c
	$(CC) -o $@ -c $(CFLAGS) $^

clean:
	$(RM) $(OBJS)

dist-clean: clean
	$(RM) vismu
