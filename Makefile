make:
	gcc ./main.c
	./a.out

strace:
	gcc ./main.c
	strace ./a.out

debug:
	gcc -g ./main.c
	gdb ./a.out
