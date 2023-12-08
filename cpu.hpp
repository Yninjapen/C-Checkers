#ifndef CPU_H
#define CPU_H

#include <algorithm>
#include <iostream>
#include <map>
#include "misc.hpp"
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
        void init_tables();
        std::map<long long, int> red_piece_map;
        std::map<long long, int> black_piece_map;
};
#endif