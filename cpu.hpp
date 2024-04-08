#ifndef CPU_H
#define CPU_H

#include <algorithm>
#include <iostream>
#include <map>
#include <unordered_map>
#include <thread>
#include <time.h>
#include "misc.hpp"
#include "board.hpp"
//VERSION 1.0
class cpu{
    int max_depth;
    int current_depth;
    int opponent;
    int eval_multiplier;
    double time_limit;

    public:
        int color;
        cpu(int cpu_color = 0, int cpu_depth = 10);
        Move max_depth_search(Board &board, bool feedback = true);
        Move time_search(Board &board, double t_limit, bool feedback = true);

        double minimax(Board &board, int depth, double alpha, double beta);
        double evaluate(Board board);

        void set_color(int new_color);
        void set_depth(int new_depth);
        void manage_time();

    private:
        const uint32_t square_map[34] = {
            (1 << 0), (1 << 1), (1 << 2), (1 << 3), (1 << 4), (1 << 5), (1 << 6), (1 << 7), (1 << 8), (1 << 9), (1 << 10), (1 << 11), (1 << 12), (1 << 13), (1 << 14), (1 << 15),
            (1 << 16), (1 << 17), (1 << 18), (1 << 19), (1 << 20), (1 << 21), (1 << 22), (1 << 23), (1 << 24), (1 << 25), (1 << 26), (1 << 27), (1 << 28), (1 << 29), (1 << 30), ((uint32_t)1 << 31),
            0, 0 //     invalid no bits set
        };
        const uint32_t DOUBLE_CORNER = square_map[3] | square_map[7] | square_map[24] | square_map[28];
        const uint32_t SINGLE_EDGE = square_map[0] | square_map[1] | square_map[2] | square_map[8] | square_map[15] | square_map[16] | square_map[23] |
                                        square_map[29] | square_map[30] | square_map[31];
        const uint32_t CENTER_8 = square_map[9] | square_map[10] | square_map[13] | square_map[14] | square_map[17] | square_map[18] | 
                                        square_map[21] | square_map[22];
        void init_tables();
        double search_start = time(NULL);
        bool search_cancelled = false;
        int nodes_traversed;
        std::unordered_map<uint32_t, int> red_piece_map;
        std::unordered_map<uint32_t, int> black_piece_map;
};

class time_manager{
    cpu &cpu1;

    public:
        time_manager(cpu &cpu_) : cpu1(cpu_){}
        void operator()() const{
            cpu1.manage_time();
        }
};
#endif