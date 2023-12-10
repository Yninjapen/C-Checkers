#ifndef CPU_H
#define CPU_H

#include <algorithm>
#include <iostream>
#include <map>
#include <unordered_map>
#include "misc.hpp"
#include "board.hpp"

class cpu{
    int max_depth;
    int color;
    int opponent;
    int eval_multiplier;

    public:
        cpu(int cpu_color = 0, int cpu_depth = 10);
        Move find_best_move(Board board);
        double minimax(Board board, int depth, double alpha, double beta);
        double evaluate(Board board);
        void set_color(int new_color);
    
    private:
        void init_tables();
        std::unordered_map<long long, int> red_piece_map;
        std::unordered_map<long long, int> black_piece_map;
};
#endif