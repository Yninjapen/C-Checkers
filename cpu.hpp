#ifndef CPU_H
#define CPU_H

#define DO_NULL    1
#define NO_NULL    0
#define IS_PV      1
#define NO_PV      0

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
        int nodes_traversed;
        cpu(int cpu_color = 0, int cpu_depth = 10);
        Move max_depth_search(Board &board, bool feedback = true);
        Move time_search(Board &board, double t_limit, bool feedback = true);
        int search_root(Board &board, int depth, int alpha, int beta);
        
        void set_color(int new_color);
        void set_depth(int new_depth);

    private:
        Move killers[1024][2];
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
        double search_start = time(NULL);
        bool search_cancelled = false;
        Move move_to_make;

        int search_iterate(Board &board);
        int search_widen(Board &board, int depth, int val);
        int search(Board &board, int depth, int ply, int alpha, int beta, int is_pv);
        int quiesce(Board &board, int ply, int alpha, int beta);
        int eval(Board board);
        int draw_eval(Board &board);
        void set_killers(Move m, int ply);
        void set_move_scores(Move * m, int movecount, int ply);
        void order_moves(int movecount, Move * m, int current);
        void order_at_root(int movecount, Move * m, int current);
        inline void check_time(){
            if (!(nodes_traversed & 4095) && !search_cancelled) search_cancelled = get_time() - search_start > time_limit;
        }
};
#endif