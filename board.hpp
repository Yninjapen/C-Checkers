#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <math.h>
#include <map>
#include <unordered_map>
#include "misc.hpp"
#include "move_tables.hpp"

class Move{

    public:
        unsigned long long reds;
        unsigned long long blacks;
        unsigned long long kings;
        int color;
        bool is_take;
        bool is_promo;

        Move(long long r = 0, long long b = 0, long long k = 0, int c = 0, bool promo = false, bool take = false);
        void get_move_info(const unsigned long long previous_pos);
        long long get_end_square(const unsigned long long previous_pos);

        bool operator <(const Move& other) const{
            return hash_bb(reds, blacks, kings, color) < hash_bb(other.reds, other.blacks, other.kings, other.color);
        }
        bool operator ==(const Move& other) const{
            return (reds == other.reds) && (blacks == other.blacks) && (kings == other.kings);
        }
};

class Board{

    const unsigned long long red_promotion_mask = 0b11110000000000000000000000000000;
    const unsigned long long black_promotion_mask = 0b00000000000000000000000000001111;
    std::unordered_map<unsigned int, int> pos_history;
    const int repetition_limit = 3; //the number of times a position can be repeated before the game is considered a draw

    public:
        unsigned long long red_bb;
        unsigned long long black_bb;
        unsigned long long king_bb;
        bool has_takes;
        int movecount;
        int turn;
        Board();

        void print_board();
        unsigned long long get_all_pieces();

        std::vector<Move> legal_moves;
        std::vector<Move> move_history;
        Move get_random_move();
        void set_random_pos(int moves_to_play);
        void push_move(Move move);
        void undo();

        int is_game_over();
        int game_over;
    
    private:
        std::vector<Move> gen_moves();
        bool can_jump(long long piece, int color);
        // long long square_to_binary(const int square);
        // int binary_to_square(const long long binary);
        // std::vector<int> serialize_bb(long long bb);
};
#endif