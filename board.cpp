#include "board.hpp"

#include "misc.hpp"
#include "transposition.hpp"

#include <iostream>
#include <bitset>
#include <random>
#include <time.h>
#include <cassert>

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

Board::Board() {
   bb.pieces[BLACK] = 0b00000000000000000000111111111111;
   bb.pieces[WHITE] = 0b11111111111100000000000000000000;
   bb.kings = 0;
   bb.stm = BLACK;

   reversible_moves = 0;
   has_takes = false;
   set_flags();
}

/* Prints a representation of the board to the console */
void Board::print() {
   char arr[32] = {' '};
   char piece_reps[5] = {'b', 'w', 'B', 'W', '-'};

   for (int i = 0; i <=32; i++){
      ePieceType pt = bb.piece_on_square(i);
      arr[i] = piece_reps[pt];
   }

   for (int i = 31; i >= 0; i-=4){
      double num = i;
      int row = ceil((num)/4);
      std::cout << "\n";
      if (row%2 == 0){
         std::cout << "  ";
      }

      for (int x = 3; x >= 0; x--){
         std::cout << arr[i - x] << "   ";
         if (!arr[i - x]){
            std::cout << " ";
         }
      }
      std::cout << "   ";
      for (int x = 3; x >= 0; x--){
         if (i - x < 10){
            std::cout << "0";
         }
         std::cout << i - x<< "  ";
      }
   }
   std::cout << "\n";
   int game_over = check_win();
   int repetition = check_repetition();
   if (repetition){
      game_over = -1;
   }
   if (!game_over){
      if (bb.stm == 0){
         std::cout << "Black to move\n";
      }
      else{
         std::cout << "White to move\n";
      }
   }
   else{
      if (game_over == 1){
        std::cout << "Black wins\n";
      }
      else if (game_over == 2){
         std::cout << "White wins\n";
      }
      else{
         if (reversible_moves >= DRAW_MOVE_RULE){
            std::cout << "Draw by 50 move rule\n";
         }
         else{
         std::cout << "Draw by repetition\n";
         }
      }
   }
}

/* Resets the board to the starting position. */
void Board::reset() {
   bb.pieces[BLACK] = 0b00000000000000000000111111111111;
   bb.pieces[WHITE] = 0b11111111111100000000000000000000;
   bb.kings = 0;
   bb.stm = BLACK;

   reversible_moves = 0;
   has_takes = false;
   set_flags();
}

/* 
Calculates a hash key for the board

@return 
   the hash key
 */
uint64_t Board::calc_hash_key() {
   uint64_t checkSum = 0;
   for (int i = 0; i < 32; i++){
      if (S[i] & bb.all_pieces()){
         checkSum ^= hash.HASH_FUNCTION[bb.piece_on_square(i)][i];
      }
   }
   if(bb.stm){
      checkSum ^= hash.HASH_COLOR;
   }
   return checkSum;
}

/* Ensure the hash_key of the board and all piece counters are correct */
void Board::set_flags(){
   hash_key = calc_hash_key();
   piece_count[0] = 0;
   piece_count[1] = 0;
   king_count[0] = 0;
   king_count[1] = 0;

   for (int i = 0; i < 32; i++){
      ePieceType pt = bb.piece_on_square(i);
      if (pt != NO_PIECE) {
         piece_count[pt & 1]++;
         if (pt > WHITE_PIECE) king_count[pt & 1]++;
      }
   }
}

/*
Plays a move on the board

@param move 
   The move to be played
*/
void Board::push_move(Move move) {
   uint8_t to = move.to;
   uint8_t from = move.from;
   uint32_t taken = move.taken_bb;

   /* Increment the move counter and the counter for consecutive reversible moves */
   uint8_t piecetype = move.piecetype;
   reversible_moves = ((piecetype <= WHITE_PIECE) || (taken)) ? 0 : reversible_moves+1;

   /* Updates the hash key of the board */
   hash_key ^= hash.HASH_COLOR;
   hash_key ^= hash.HASH_FUNCTION[piecetype][from];

   /* Loop through taken pieces and do all necessary handling */
   while (taken) {
      uint32_t piece = taken & -taken;
      uint8_t taken_piecetype = (!bb.stm) + 2*(!!(piece & bb.kings));

      hash_key ^= hash.HASH_FUNCTION[taken_piecetype][binary_to_square(piece)]; // Update the board's hash for the removed piece
      piece_count[!bb.stm]--; // Decrement the piece counter
      if (taken_piecetype > WHITE_PIECE) // If the piece was a king, decrement the king counter
         king_count[!bb.stm]--;

      taken &= taken - 1;
   }

   bb.pieces[bb.stm] ^= S[from];        // Remove the piece that is being moved from its start square
   bb.pieces[bb.stm] |= S[to];          // Put it back down on the square it lands
   bb.pieces[!bb.stm] ^= move.taken_bb; // Remove all taken pieces from the board
   bb.kings &= ~move.taken_bb;          // Remove all taken pieces from the king bitboard

   if (move.is_promo) {
      piecetype += 2;       // Change the piecetype to a king
      king_count[bb.stm]++; // Increment King counter
      bb.kings |= S[to];    // Add the piece to the king bitboard
   }
   else if (move.is_king()) {
      bb.kings ^= S[from];  // Remove the piece that is being moved from its start square on the king bitboard
      bb.kings |= S[to];    // Put it back down on the square it lands
   }

   bb.stm = !bb.stm; // Switch the side to move

   hash_key ^= hash.HASH_FUNCTION[piecetype][to]; // Update the board's hash

   /* Updates the repetition tracker */
   if (bb.kings) rep_stack[reversible_moves] = hash_key; // Add the hash to the repetition list
}

/*
Undoes a move

@param move 
   Move to be undone
@param previous_kings 
   The king bitboard from the previous position
*/
void Board::undo(Move move, uint32_t previous_kings) {
   if (reversible_moves) reversible_moves--;

   uint8_t to = move.to;
   uint8_t from = move.from;

   uint8_t piecetype = move.piecetype;
   hash_key ^= hash.HASH_COLOR;
   hash_key ^= hash.HASH_FUNCTION[piecetype][from];

   uint32_t taken = move.taken_bb;
   while (taken) {
      uint32_t piece = taken & -taken;
      uint8_t taken_piecetype = bb.stm;
      if (piece & previous_kings) {
         taken_piecetype += 2;
         king_count[bb.stm]++;
      }

      hash_key ^= hash.HASH_FUNCTION[taken_piecetype][binary_to_square(piece)];
      piece_count[bb.stm]++;
      taken &= taken - 1;
   }

   bb.pieces[bb.stm] |= taken;
   bb.kings |= previous_kings;
   bb.pieces[!bb.stm] |= S[from];
   bb.pieces[!bb.stm] ^= S[to];

   if (move.is_promo) {
      bb.kings ^= S[to];
      piecetype += 2;
      king_count[!bb.stm]--;
   }
   else if (move.is_king()) {
      bb.kings ^= S[to];
      bb.kings |= S[from];
   }

   bb.stm = !bb.stm;

   hash_key ^= hash.HASH_FUNCTION[piecetype][to];
}

/*
If the "jumper" can jump, add the jump to the movelist and return true. Return false if it cannot jump. Use this method for black pieces.

@param start_square
   Where the jump initially started
@param current_square
   Where the piece jumping currently is in its jump
@param captures
   How many pieces this jump has taken so far
@param taken_bb
   A bitboard of all the pieces that this jump has taken so far

@return
   true if the piece can jump again, false if the piece cannot jump again
*/
bool Board::add_black_jump(uint32_t start_square, uint32_t current_square, uint8_t captures, uint32_t taken_bb){
   const uint32_t temp_black = (bb.pieces[BLACK] ^ start_square) | current_square;
   const uint32_t temp_white = bb.pieces[WHITE] ^ taken_bb;
   const uint32_t empty = ~(temp_black | temp_white);
   const int new_captures = captures + 1;
   uint32_t taken, dest;

   bool result = false;

   // Does a quick check to see whether this piece can even jump
   uint32_t temp = (empty >> 4) & temp_white;
   uint32_t jump_check = (((temp & MASK_R3) >> 3) | ((temp & MASK_R5) >> 5));
   temp = (((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & temp_white;
   jump_check |= (temp >> 4);

   if (start_square & bb.kings) {
      /* Finish checking if this piece can move */
      temp = (empty << 4) & temp_white;
      jump_check |= (((temp & MASK_L3) << 3) | ((temp & MASK_L5) << 5));
      temp = (((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & temp_white;
      jump_check |= (temp << 4);
      if (!(current_square & jump_check)) return false;

      taken = (current_square >> 4) & temp_white;
      dest = (((taken & MASK_R3) >> 3) | ((taken & MASK_R5) >> 5)) & empty;
      if (taken && dest) {
         if (!add_black_jump(start_square, dest, new_captures, taken_bb | taken))
            movegen_push(start_square, dest, new_captures, taken_bb | taken);
         result = true;
      }
      taken = (((current_square & MASK_R3) >> 3) | ((current_square & MASK_R5) >> 5)) & temp_white;
      dest = (taken >> 4) & empty;
      if (taken && dest) {
         if (!add_black_jump(start_square, dest, new_captures, taken_bb | taken))
            movegen_push(start_square, dest, new_captures, taken_bb | taken);
         result = true;
      }
   }
   else
      if (!(current_square & jump_check)) return false;

   taken = (current_square << 4) & temp_white;
   dest = (((taken & MASK_L3) << 3) | ((taken & MASK_L5) << 5)) & empty;
   if (taken && dest) {
      if (!add_black_jump(start_square, dest, new_captures, taken_bb | taken))
         movegen_push(start_square, dest, new_captures, taken_bb | taken);
      result = true;
   }
   taken = (((current_square & MASK_L3) << 3) | ((current_square & MASK_L5) << 5)) & temp_white;
   dest = (taken << 4) & empty;
   if (taken && dest) {
      if (!add_black_jump(start_square, dest, new_captures, taken_bb | taken))
         movegen_push(start_square, dest, new_captures, taken_bb | taken);
      result = true;
   }
   return result;
}

/*
If the "jumper" can jump, add the jump to the movelist and return true. Return false if it cannot jump. Use this method for white pieces.

@param start_square
   Where the jump initially started
@param current_square
   Where the piece jumping currently is in its jump
@param captures
   How many pieces this jump has taken so far
@param taken_bb
   A bitboard of all the pieces that this jump has taken so far

@return
   true if the piece can jump again, false if the piece cannot jump again
*/
bool Board::add_white_jump(uint32_t start_square, uint32_t current_square, uint8_t captures, uint32_t taken_bb){
   const uint32_t temp_black = bb.pieces[BLACK] ^ taken_bb;
   const uint32_t temp_white = (bb.pieces[WHITE] ^ start_square) | current_square;
   const uint32_t empty = ~(temp_black | temp_white);
   const int new_captures = captures + 1;
   uint32_t taken, dest;

   bool result = false;

   // Does a quick check to see whether this piece can even jump
   uint32_t temp = (empty << 4) & temp_black;
   uint32_t jump_check = (((temp & MASK_L3) << 3) | ((temp & MASK_L5) << 5));
   temp = (((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & temp_black;
   jump_check |= (temp << 4);

   if (start_square & bb.kings){
      /* Finish checking if this piece can move */
      temp = (empty >> 4) & temp_black;
      jump_check |= (((temp & MASK_R3) >> 3) | ((temp & MASK_R5) >> 5));
      temp = (((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & temp_black;
      jump_check |= (temp >> 4);

      if (!(current_square & jump_check)) return false;
      
      taken = (current_square << 4) & temp_black;
      dest = (((taken & MASK_L3) << 3) | ((taken & MASK_L5) << 5)) & empty;
      if (taken && dest) {
         if (!add_white_jump(start_square, dest, new_captures, taken_bb | taken))
            movegen_push(start_square, dest, new_captures, taken_bb | taken);
         result = true;
      }
      taken = (((current_square & MASK_L3) << 3) | ((current_square & MASK_L5) << 5)) & temp_black;
      dest = (taken << 4) & empty;
      if (taken && dest) {
         if (!add_white_jump(start_square, dest, new_captures, taken_bb | taken))
            movegen_push(start_square, dest, new_captures, taken_bb | taken);
         result = true;
      }
   }
   else
      if (!(current_square & jump_check)) return false;
   
   taken = (current_square >> 4) & temp_black;
   dest = (((taken & MASK_R3) >> 3) | ((taken & MASK_R5) >> 5)) & empty;
   if (taken && dest) {
      if (!add_white_jump(start_square, dest, new_captures, taken_bb | taken))
         movegen_push(start_square, dest, new_captures, taken_bb | taken);
      result = true;
   }
   taken = (((current_square & MASK_R3) >> 3) | ((current_square & MASK_R5) >> 5)) & temp_black;
   dest = (taken >> 4) & empty;
   if (taken && dest) {
      if (!add_white_jump(start_square, dest, new_captures, taken_bb | taken))
         movegen_push(start_square, dest, new_captures, taken_bb | taken);
      result = true;
   }
   return result;
}

/*
Generates all legal moves and puts them in the external_movelist array that is passed in.
@param external_movelist
   an array that will be populated by all the legal moves
@param tt_move
   the id of the best move fetched from the transposition table
@return
   the number of legal moves in the position
*/
int Board::gen_moves(Move * external_movelist, uint8_t tt_move){
   has_takes = false;
   legal_move_count = 0;
   movelist = external_movelist;
   const uint32_t empty = ~(bb.all_pieces());

   if (bb.stm) { // White Moves
      uint32_t jumpers = bb.get_white_jumpers();
      if (jumpers) { // White Jumps
         has_takes = true;
         uint32_t piece, taken, dest;

         while (jumpers) { //Loop through white pieces that can jump
            piece = jumpers & -jumpers;
            taken = (piece >> 4) & bb.pieces[BLACK];
            dest = (((taken & MASK_R3) >> 3) | ((taken & MASK_R5) >> 5)) & empty;
            if (taken && dest) {
               if (!add_white_jump(piece, dest, 1, taken))
                  movegen_push(piece, dest, 1, taken);
            }
            taken = (((piece & MASK_R3) >> 3) | ((piece & MASK_R5) >> 5)) & bb.pieces[BLACK];
            dest = (taken >> 4) & empty;
            if (taken && dest) {
               if (!add_white_jump(piece, dest, 1, taken))
                  movegen_push(piece, dest, 1, taken);
            }
            if (piece & bb.kings) {
               taken = (piece << 4) & bb.pieces[BLACK];
               dest = (((taken & MASK_L3) << 3) | ((taken & MASK_L5) << 5)) & empty;
               if (taken && dest) {
                  if (!add_white_jump(piece, dest, 1, taken))
                     movegen_push(piece, dest, 1, taken);
               }
               taken = (((piece & MASK_L3) << 3) | ((piece & MASK_L5) << 5)) & bb.pieces[BLACK];
               dest = (taken << 4) & empty;
               if (taken && dest) {
                  if (!add_white_jump(piece, dest, 1, taken))
                     movegen_push(piece, dest, 1, taken);
               }
            }
            jumpers &= jumpers-1;
         } // White Jumpers Loop
      } // White Jump Moves
      else { // White Non-Capture Moves
         uint32_t movers = bb.get_white_movers();
         uint32_t piece, dest;
         while (movers) { // Loop Through Non-Captures
            piece = movers & -movers;
            dest = (piece >> 4) & empty;
            if (dest) 
               movegen_push(piece, dest, 0, 0);
            dest = (((piece & MASK_R3) >> 3) | ((piece & MASK_R5) >> 5)) & empty;
            if (dest)
               movegen_push(piece, dest, 0, 0);
            if (piece & bb.kings) {
               dest = (piece << 4) & empty;
               if (dest)
                  movegen_push(piece, dest, 0, 0);
               dest = (((piece & MASK_L3) << 3) | ((piece & MASK_L5) << 5)) & empty;
               if (dest)
                  movegen_push(piece, dest, 0, 0);
            }
            movers &= movers-1;
         } // White Non-Captures Loop
      } // White Non-Captures
   } // White Moves

   else { // Black Moves
      uint32_t jumpers = bb.get_black_jumpers();
      if (jumpers) { // Black Jumps
         has_takes = true;
         uint32_t piece, taken, dest;

         while (jumpers) { // Loop through black pieces that can jump
            piece = jumpers & -jumpers;
            taken = (piece << 4) & bb.pieces[WHITE];
            dest = (((taken & MASK_L3) << 3) | ((taken & MASK_L5) << 5)) & empty;
            if (taken && dest) {
               if (!add_black_jump(piece, dest, 1, taken))
                  movegen_push(piece, dest, 1, taken);
            }
            taken = (((piece & MASK_L3) << 3) | ((piece & MASK_L5) << 5)) & bb.pieces[WHITE];
            dest = (taken << 4) & empty;
            if (taken && dest) {
               if (!add_black_jump(piece, dest, 1, taken))
                  movegen_push(piece, dest, 1, taken);
            }
            if (piece & bb.kings) {
               taken = (piece >> 4) & bb.pieces[WHITE];
               dest = (((taken & MASK_R3) >> 3) | ((taken & MASK_R5) >> 5)) & empty;
               if (taken && dest) {
                  if (!add_black_jump(piece, dest, 1, taken))
                     movegen_push(piece, dest, 1, taken);
               }
               taken = (((piece & MASK_R3) >> 3) | ((piece & MASK_R5) >> 5)) & bb.pieces[WHITE];
               dest = (taken >> 4) & empty;
               if (taken && dest) {
                  if (!add_black_jump(piece, dest, 1, taken))
                     movegen_push(piece, dest, 1, taken);
               }
            }
            jumpers &= jumpers-1;
         } // Black Jumpers Loop
      } // Black Jump Moves

      else { // Black Non-Captures
         uint32_t movers = bb.get_black_movers();
         uint32_t piece, dest;
         while (movers) { // Loop through Non-Captures
            piece = movers & -movers;
            dest = (piece << 4) & empty;
            if (dest)
               movegen_push(piece, dest, 0, 0);
            dest = (((piece & MASK_L3) << 3) | ((piece & MASK_L5) << 5)) & empty;
            if (dest)
               movegen_push(piece, dest, 0, 0);
            if (piece & bb.kings) {
               dest = (piece >> 4) & empty;
               if (dest)
                  movegen_push(piece, dest, 0, 0);
               dest = (((piece & MASK_R3) >> 3) | ((piece & MASK_R5) >> 5)) & empty;
               if (dest)
                  movegen_push(piece, dest, 0, 0);
            }
            movers &= movers-1;
         } // Black Non-Captures Loop
      } // Black Non-Captures
   } // Black Moves

   /* If a preferred best move is passed in, boost the score of that move. */
   if ((tt_move != -1) && (tt_move < legal_move_count)) external_movelist[tt_move].score = HASH_SORT;

   return legal_move_count;
}

/*
Get a random move. Note that the random
seed must be set before this is called.

@return
   A random legal move
*/
Move Board::get_random_move(){
   Move arr[64];
   gen_moves(arr, (char)-1);
   int index = rand() % legal_move_count;
   return arr[index];
}

/*
Sets the board to a random position by playing "moves_to_play" random moves.
Note that the random seed must be set before calling this method.

@param moves_to_play
   How many moves should be played to get to the random position
*/
void Board::set_random_pos(int moves_to_play){
   for (int i = 0; i < moves_to_play; i++){
      push_move(get_random_move());
   }
}

/*
Checks if the current position has a winner. Use check_repetition for checking draws.

@return
   an int that represents the result. If the game is not over, return 0. A Black win returns 1, a White win returns 2.
*/
int Board::check_win() const{
   if (legal_move_count == 0){
      //                         (1)             (0)
      return 2 - bb.stm; // 2 - WHITE == 1, 2 - BLACK == 2
   }
   return 0; //otherwise, the game is not over
}

/*
Checks whether the current position is a draw by repetition or 50 move rule.

@return
   true if the current position has been repeated at least once or if 
   the game is a draw by 50 move rule (50 moves without take or promotion)
*/
bool Board::check_repetition() const{
   if (!bb.kings) return false;
   if (reversible_moves >= DRAW_MOVE_RULE) return true;
   int i = 0;
   if (reversible_moves & 1) i++;

   for(; i < reversible_moves-1; i+=2){
      if (rep_stack[i] == hash_key) return true;
   }
   return false;
}

/* Prints the start and end square of the move, as well as any taken squares */
void Move::print_move_info() {
   std::cout << (int)from;
   uint32_t taken = taken_bb;
   while (taken) {
      std::cout << "-" << binary_to_square(taken & -taken);
      taken &= taken-1;
   }
   std::cout << "-" << (int)to;
}