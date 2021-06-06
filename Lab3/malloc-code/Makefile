alloc: test_alloc.c alloc.cpp
	g++ test_alloc.c alloc.cpp -o alloc

ealloc: ealloc.cpp test_ealloc.c
	g++ -w test_ealloc.c ealloc.cpp -o ealloc

clean:
	rm -f alloc ealloc a.out
