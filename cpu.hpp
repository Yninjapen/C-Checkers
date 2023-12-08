#include <algorithm>
#include <iostream>
#include "board.hpp"

class cpu{
    int max_depth;
    int color;
    int opponent;
    int eval_multiplier;

    public:
        cpu(int cpu_color, int cpu_depth);
        Move find_best_move(Board board);
        double minimax(Board board, int depth, double alpha, double beta, bool isMaximizingPlayer);
        double evaluate(Board board);
    
    private:
        int count_bits(long long bb);
};