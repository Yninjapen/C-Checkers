//https://3dkingdoms.com/checkers/bitboards.htm
//https://github.com/jonkr2/GuiNN_Checkers/blob/main/src

#pragma once

#include "misc.hpp"

#include <cstdint>
#include <cassert>

#define TAKE_SORT 10000
#define PROMO_SORT 15000
#define KILLER_SORT 100000
#define HASH_SORT 200000
#define INVALID 32767

#define MAX_VAL 10000
#define MAX_MOVES 28

/* Board Representation
         WHITE
   28    29    30    31
24    25    26    27  
   20    21    22    23
16    17    18    19  
   12    13    14    15
08    09    10    11  
   04    05    06    07
00    01    02    03  
         BLACK
*/
/* Translates a square number into its bit representation*/
const uint32_t S[32] = {
            (1 << 0), (1 << 1), (1 << 2), (1 << 3), (1 << 4), (1 << 5), (1 << 6), (1 << 7), (1 << 8), (1 << 9), (1 << 10), (1 << 11), (1 << 12), (1 << 13), (1 << 14), (1 << 15),
            (1 << 16), (1 << 17), (1 << 18), (1 << 19), (1 << 20), (1 << 21), (1 << 22), (1 << 23), (1 << 24), (1 << 25), (1 << 26), (1 << 27), (1 << 28), (1 << 29), (1 << 30), ((uint32_t)1 << 31)
};

const uint32_t MASK_L3 = S[ 1] | S[ 2] | S[ 3] | S[ 9] | S[10] | S[11] | S[17] | S[18] | S[19] | S[25] | S[26] | S[27];
const uint32_t MASK_L5 = S[ 4] | S[ 5] | S[ 6] | S[12] | S[13] | S[14] | S[20] | S[21] | S[22];
const uint32_t MASK_R3 = S[28] | S[29] | S[30] | S[20] | S[21] | S[22] | S[12] | S[13] | S[14] | S[ 4] | S[ 5] | S[ 6];
const uint32_t MASK_R5 = S[25] | S[26] | S[27] | S[17] | S[18] | S[19] | S[ 9] | S[10] | S[11];

const uint32_t RANK[8] = {
    S[0]  | S[1]  | S[2]  | S[3],
    S[4]  | S[5]  | S[6]  | S[7],
    S[8]  | S[9]  | S[10] | S[11],
    S[12] | S[13] | S[14] | S[15],
    S[16] | S[17] | S[18] | S[19],
    S[20] | S[21] | S[22] | S[23],
    S[24] | S[25] | S[26] | S[27],
    S[28] | S[29] | S[30] | S[31]
};

const uint32_t PROMO_MASK[2] = {RANK[7], RANK[0]};

const int DRAW_MOVE_RULE = 50;
const int REP_LIMIT = 3;

const uint64_t TAKEN_PIECES = (((uint64_t)1 << 32) - 1) << 17;

enum eColor {
    BLACK,
    WHITE
};

enum ePieceType {
    BLACK_PIECE,
    WHITE_PIECE,
    BLACK_KING,
    WHITE_KING,
    NO_PIECE
};

struct Move {
    uint8_t from;
    uint8_t to;
    uint8_t piecetype;
    uint8_t captures;
    uint32_t taken_bb;
    bool is_promo;
    int score;
    uint8_t id;

    inline uint8_t color() const { return piecetype & 1; }
    inline bool is_king() const { return piecetype & 2; }

    /* Make sure to set data to 0 before calling any of these */
    inline void set_color(uint8_t color) { piecetype = (piecetype & 2) | color; }
    inline void set_is_king(bool is_king) { piecetype = (piecetype & 1) | (is_king << 1); }

    void print_move_info();
};

struct Bitboards {
    uint32_t pieces[2]; // Bitboards of the Black and White pieces
    uint32_t kings;     // Bitboard for kings for both sides
    uint8_t stm;        // Side to move

    inline uint32_t all_pieces() const {
        return pieces[BLACK] | pieces[WHITE];
    }
    inline ePieceType piece_on_square(int square) {
        if (S[square] & pieces[BLACK]) {
            if (S[square] & kings) return BLACK_KING;
            return BLACK_PIECE;
        }
        else if (S[square] & pieces[WHITE]) {
            if (S[square] & kings) return WHITE_KING;
            return WHITE_PIECE;
        }
        return NO_PIECE;
    }
    inline ePieceType piecetype(uint32_t piece) {
        if (piece & pieces[BLACK]) {
            if (piece & kings) return BLACK_KING;
            return BLACK_PIECE;
        }
        if (piece & kings) {
            return WHITE_KING;
        }
        return WHITE_PIECE;
    }
    inline uint32_t get_black_movers() const {
        const uint32_t empty = ~(pieces[BLACK] | pieces[WHITE]);
        const uint32_t black_kings = pieces[BLACK] & kings;
        uint32_t result = ((((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) | (empty >> 4)) & pieces[BLACK];
        if (black_kings) {
            result |= ((((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) | (empty << 4)) & black_kings;
        }
        return result;
    }
    inline uint32_t get_white_movers() const {
        const uint32_t empty = ~(pieces[BLACK] | pieces[WHITE]);
        const uint32_t white_kings = pieces[WHITE] & kings;
        uint32_t result = ((((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) | (empty << 4)) & pieces[WHITE];
        if (white_kings) {
            result |= ((((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) | (empty >> 4)) & white_kings;
        }
        return result;
    }
    inline uint32_t get_black_jumpers() const {
        const uint32_t empty = ~(pieces[BLACK] | pieces[WHITE]);
        const uint32_t black_kings = pieces[BLACK] & kings;

        uint32_t temp = (empty >> 4) & pieces[WHITE];
        uint32_t jumpers = (((temp & MASK_R3) >> 3) | ((temp & MASK_R5) >> 5));
        temp = (((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & pieces[WHITE];
        jumpers |= (temp >> 4);
        jumpers &= pieces[BLACK];

        if (black_kings) {
            temp = (empty << 4) & pieces[WHITE];
            jumpers |= (((temp & MASK_L3) << 3) | ((temp & MASK_L5) << 5)) & black_kings;
            temp = (((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & pieces[WHITE];
            jumpers |= (temp << 4) & black_kings;
        }
        return jumpers;
    }
    inline uint32_t get_white_jumpers() const {
        const uint32_t empty = ~(pieces[BLACK] | pieces[WHITE]);
        const uint32_t white_kings = pieces[WHITE] & kings;

        uint32_t temp = (empty << 4) & pieces[BLACK];
        uint32_t jumpers = (((temp & MASK_L3) << 3) | ((temp & MASK_L5) << 5));
        temp = (((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & pieces[BLACK];
        jumpers |= (temp << 4);
        jumpers &= pieces[WHITE];
        if (white_kings) {
            temp = (empty >> 4) & pieces[BLACK];
            jumpers |= (((temp & MASK_R3) >> 3) | ((temp & MASK_R5) >> 5)) & white_kings;
            temp = (((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & pieces[BLACK];
            jumpers |= (temp >> 4) & white_kings;
        }
        return jumpers;
    }
};

struct Board {
    public:
        Bitboards bb;
        uint8_t piece_count[2];
        uint8_t king_count[2];

        bool has_takes;
        int reversible_moves;
        uint64_t hash_key;
        uint64_t rep_stack[DRAW_MOVE_RULE];

        Board();

        void reset();
        void print();

        Move get_random_move();
        void set_random_pos(int moves_to_play);

        void push_move(Move move);
        void undo(Move move, uint32_t previous_kings);
        int gen_moves(Move * external_movelist, uint8_t tt_move);
        int check_win() const;
        bool check_repetition() const;

        inline void clear_pos_history() {
            reversible_moves = 0;
        }

    private:
        /* Number of legal moves on the board */
        int legal_move_count;

        Move * movelist;

        bool add_black_jump(uint32_t start_square, uint32_t current_square, uint8_t captures, uint32_t taken_bb);
        bool add_white_jump(uint32_t start_square, uint32_t current_square, uint8_t captures, uint32_t taken_bb);

        void set_flags();
        uint64_t calc_hash_key();

        /*
        Returns a score for a piece based on how far it is up the
        board relative to its color. Used mainly for move ordering.
        */
        inline int get_tempo_score(uint32_t piece, char color) const {
            if (color){
                    if (piece & RANK[3]) return 1;
                    if (piece & RANK[2]) return 2;
                    if (piece & RANK[1]) return 3;
            }
            else{
                    if (piece & RANK[4]) return 1;
                    if (piece & RANK[5]) return 2;
                    if (piece & RANK[6]) return 3;
            }
            return 0;
        }
        inline void movegen_push(uint32_t from, uint32_t to, uint8_t captures, uint32_t taken_bb) {
            movelist[legal_move_count].from = binary_to_square(from);
            movelist[legal_move_count].to = binary_to_square(to);
            movelist[legal_move_count].set_color(bb.stm);
            movelist[legal_move_count].captures = captures;
            movelist[legal_move_count].taken_bb = taken_bb;
            movelist[legal_move_count].is_promo = false;
            movelist[legal_move_count].score = 0;

            bool is_king = from & bb.kings;
            movelist[legal_move_count].set_is_king(is_king);
            if (!is_king) {
                movelist[legal_move_count].is_promo = to & PROMO_MASK[bb.stm];
                movelist[legal_move_count].score += get_tempo_score(to, bb.stm);
            }

            if (movelist[legal_move_count].is_promo) movelist[legal_move_count].score += PROMO_SORT;

            movelist[legal_move_count].score += captures * TAKE_SORT;
            movelist[legal_move_count].id = legal_move_count;
            legal_move_count++;
        }
};