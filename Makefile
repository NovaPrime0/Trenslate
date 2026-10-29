#for bash
targetfiles := $(wildcard *.c)
output := $(targetfiles:c=o)
CFLAGS = -Wall -Wextra -g 
do:
	make run;
	make clean -s;
main.exe: $(output)
	gcc $^ -o main 
%.o: %.c
	gcc $(CFLAGS) -c $^ 
run: main.exe
	./$^
clean: 
	rm -f *.o main

main:
	make run
	rm -f *.o
.PHONY: all run clean