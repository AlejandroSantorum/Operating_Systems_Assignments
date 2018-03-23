CC = gcc -ansi -pedantic -std=gnu99
CFLAGS = -Wall -g
EXE = ejercicio2 ejercicio4 ejercicio6a ejercicio6b

all : $(EXE)

.PHONY : clean
clean :
	rm -f *.o core $(EXE)

$(EXE) : % : %.o
	$(CC) $(CFLAGS) -o $@ $@.o
