//https://3dkingdoms.com/checkers/bitboards.htm
//https://github.com/jonkr2/GuiNN_Checkers/blob/main/src

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
        uint32_t reds;
        uint32_t blacks;
        uint32_t kings;
        int color;
        bool is_take;
        bool is_promo;

        Move(uint32_t r = 0, uint32_t b = 0, uint32_t k = 0, int c = 0, bool promo = false, bool take = false);
        void get_move_info(const uint32_t previous_pos);
        uint32_t get_end_square(const uint32_t previous_pos);

        bool operator <(const Move& other) const{
            return hash_bb(reds, blacks, kings, color) < hash_bb(other.reds, other.blacks, other.kings, other.color);
        }
        bool operator ==(const Move& other) const{
            return (reds == other.reds) && (blacks == other.blacks) && (kings == other.kings);
        }
};

/* Bitboard configuration
    29    30    31    32
 25    26    27    28  
    21    22    23    24
 17    18    19    20
    13    14    15    16
 09    10    11    12  
    05    06    07    08
 01    02    03    04
*/
class Board{

    const uint32_t red_promotion_mask = 0b11110000000000000000000000000000;
    const uint32_t black_promotion_mask = 0b00000000000000000000000000001111;
    std::unordered_map<unsigned int, int> pos_history;
    const int repetition_limit = 3; //the number of times a position can be repeated before the game is considered a draw
    const int max_moves_without_take = 50;

    public:
        const uint32_t S[34] = {
            (1 << 0), (1 << 1), (1 << 2), (1 << 3), (1 << 4), (1 << 5), (1 << 6), (1 << 7), (1 << 8), (1 << 9), (1 << 10), (1 << 11), (1 << 12), (1 << 13), (1 << 14), (1 << 15),
            (1 << 16), (1 << 17), (1 << 18), (1 << 19), (1 << 20), (1 << 21), (1 << 22), (1 << 23), (1 << 24), (1 << 25), (1 << 26), (1 << 27), (1 << 28), (1 << 29), (1 << 30), ((uint32_t)1 << 31),
            0, 0 //     invalid no bits set
        };
        uint32_t red_bb;
        uint32_t black_bb;
        uint32_t king_bb;
        bool has_takes;
        int movecount;
        int turn;
        Board();

        void print_board();
        uint32_t get_all_pieces();

        std::vector<Move> legal_moves;
        Move get_random_move();
        void set_random_pos(int moves_to_play);
        void push_move(Move move);
        void undo(Move prev_pos, Move curr_pos);

        int is_game_over();
        int game_over;
    
    private:
        int moves_since_take;
        const uint32_t MASK_L3 = S[ 1] | S[ 2] | S[ 3] | S[ 9] | S[10] | S[11] | S[17] | S[18] | S[19] | S[25] | S[26] | S[27];
        const uint32_t MASK_L5 = S[ 4] | S[ 5] | S[ 6] | S[12] | S[13] | S[14] | S[20] | S[21] | S[22];
        const uint32_t MASK_R3 = S[28] | S[29] | S[30] | S[20] | S[21] | S[22] | S[12] | S[13] | S[14] | S[ 4] | S[ 5] | S[ 6];
        const uint32_t MASK_R5 = S[25] | S[26] | S[27] | S[17] | S[18] | S[19] | S[ 9] | S[10] | S[11];;
        std::vector<Move> gen_moves();
        bool can_jump(uint32_t piece, int color);
        uint32_t get_red_movers();
        uint32_t get_black_movers();
        uint32_t get_red_jumpers();
        uint32_t get_black_jumpers();
};
#endif