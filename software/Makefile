
all: versat.h testbench.c ./pc/versat.c
	g++ -pg -o out.exe *.c ./pc/versat.c
	./out.exe

clean:
	rm -f out.exe
	rm -f gmon.out

.PHONY: all clean