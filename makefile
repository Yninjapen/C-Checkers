CFLAGS = -march=native

game: 
	g++ main.cpp misc.cpp board.cpp cpu.cpp $(CFLAGS)