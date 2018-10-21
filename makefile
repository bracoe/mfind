# The makefile used to compile mfind
#
# Created on: 5 Oct 2018
#     Author: Bram Coenen (tfy15bcn)
#

# Which compiler
CC = gcc

# Options for development
CFLAGS = -g -std=gnu11 -Wall -Wextra -Werror -Wmissing-declarations \
 -Wmissing-prototypes -Werror-implicit-function-declaration -Wreturn-type \
 -Wparentheses -Wunused -Wold-style-definition -Wundef -Wshadow \
 -Wstrict-prototypes -Wswitch-default -Wunreachable-code
 
LFLAGS = -lpthread

OBJ = mfind.o list.o

#make program
all:mfind

mfind: $(OBJ)
	$(CC) $(LFLAGS) $(OBJ) -o mfind

mfind.o: mfind.c list.h
	$(CC) $(CFLAGS) mfind.c -c
	
list.o: list.c list.h
	$(CC) $(CFLAGS) list.c -c

#Other options
.PHONY: clean valgrind

clean:
	rm -f $(OBJ)

valgrind: all
	valgrind --leak-check=full --track-origins=yes ./mfind