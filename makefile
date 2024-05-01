CFLAGS = -march=native

game: 
	g++ $(CFLAGS) -o checkers main.cpp misc.cpp transposition.cpp board.cpp cpu.cpp new_cpu.cpp

comp:
	g++ $(CFLAGS) -o comp cpu_comparison.cpp misc.cpp transposition.cpp board.cpp cpu.cpp new_cpu.cpp original_cpu.cpp

test:
	g++ $(CFLAGS) -o test benchmark.cpp misc.cpp transposition.cpp board.cpp