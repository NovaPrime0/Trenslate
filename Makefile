ifeq ($(OS),Windows_NT)
	RM = del /f /q
	EXE = main.exe
else
	RM = rm -f
	EXE = main
endif

targetfiles := $(wildcard *.c)
output := $(targetfiles:.c=.o)
CFLAGS = -Wall -Wextra -g 
do:
	$(MAKE) run
	$(MAKE) clean -s
main.exe: $(output)
	gcc $^ -o $(EXE)
%.o: %.c
	gcc $(CFLAGS) -c $<
run: main.exe
	./$(EXE)
clean: 
	$(RM) *.o $(EXE)

main: main.exe
	$(RM) *.o
.PHONY: all run clean