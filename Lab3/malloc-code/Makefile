alloc: test_alloc.c 9_alloc.cpp
	g++ test_alloc.c 9_alloc.cpp -o alloc

ealloc: 9_ealloc.cpp test_ealloc.c
	g++ -w test_ealloc.c 9_ealloc.cpp -o ealloc

clean:
	rm -f alloc ealloc a.out
