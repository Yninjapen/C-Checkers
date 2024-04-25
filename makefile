CFLAGS = -march=native

game: 
	g++ $(CFLAGS) -o checkers main.cpp misc.cpp board.cpp cpu.cpp

comp:
	g++ -o comp cpu_comparison.cpp misc.cpp board.cpp cpu.cpp new_cpu.cpp original_cpu.cpp

test:
	g++ -o test benchmark.cpp misc.cpp board.cpp