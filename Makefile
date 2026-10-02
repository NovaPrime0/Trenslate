#for bash
targetfiles := $(wildcard *.c)
output := $(targetfiles:c=o)
do:
	make run;
	make clean -s;
main.exe: $(output)
	gcc $^ -o main 
%.o: %.c
	gcc -c $^ 
run: main.exe
	./$^
clean: 
	rm -f *.o main
.PHONY: all run clean