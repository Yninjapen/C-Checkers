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

class cpu{
    int max_depth;
    int current_depth;
    int opponent;
    int eval_multiplier;
    double time_limit;

    public:
        int color;
        cpu(int cpu_color = 0, int cpu_depth = 10);
        Move max_depth_search(Board board, bool feedback = true);
        Move time_search(Board board, double t_limit, bool feedback = true);

        double minimax(Board board, int depth, double alpha, double beta);
        double evaluate(Board board);

        void set_color(int new_color);
        void set_depth(int new_depth);
        void manage_time();

    private:
        void init_tables();
        double search_start = time(NULL);
        bool search_cancelled = false;
        std::unordered_map<long long, int> red_piece_map;
        std::unordered_map<long long, int> black_piece_map;
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