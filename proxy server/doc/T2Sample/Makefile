CC = gcc
CFLAGS = -lpthread
all: sample1 sample2 sample3

sample1:sample1.c
	gcc -o $@ $< $(CFLAGS)
sample2:sample2.c
	gcc -o $@ $< $(CFLAGS)
sample3:sample3.c
	gcc -o $@ $< $(CFLAGS)

clean:
	rm sample1 sample2 sample3
