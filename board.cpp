#include <iostream>
#include <bitset>
#include <random>
#include <time.h>
#include "board.hpp"

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

Board::Board(){
   red_bb = 0b00000000000000000000111111111111;
   black_bb = 0b11111111111100000000000000000000;
   king_bb = 0b00000000000000000000000000000000;
   turn = 0;
   has_takes = false;

   moves_played = 0;
   movecount = 0;
   moves_since_take = 0;
}

uint32_t Board::get_all_pieces(){
   return red_bb | black_bb;
}

//prints a representation of the board to the console
void Board::print_board(){
   char arr[32] = {' '};
   uint32_t red_kings = red_bb & king_bb;
   uint32_t red_pieces = red_bb & ~king_bb;
   uint32_t black_kings = black_bb & king_bb;
   uint32_t black_pieces = black_bb & ~king_bb;
   std::vector<int> v;

   for (int i = 0; i <=32; i++){arr[i] = '-';}
   v = serialize_bb(red_kings);for(int i = 0; i < v.size(); i++){arr[v[i] - 1] = 'R';}
   v = serialize_bb(red_pieces);for(int i = 0; i < v.size(); i++){arr[v[i] - 1] = 'r';}
   v = serialize_bb(black_kings);for(int i = 0; i < v.size(); i++){arr[v[i] - 1] = 'B';}
   v = serialize_bb(black_pieces);for(int i = 0; i < v.size(); i++){arr[v[i] - 1] = 'b';}

   for (int i = 31; i >= 0; i-=4){
      double num = i + 1;
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
         if (i - x + 1< 10){
            std::cout << "0";
         }
         std::cout << i - x + 1<< "  ";
      }
   }
   std::cout << "\n";
   int game_over = check_win();
   int repetition = check_repetition();
   if (repetition){
      game_over = -1;
   }
   if (!game_over){
      if (turn == 0){
         std::cout << "Red to move\n";
      }
      else{
         std::cout << "Black to move\n";
      }
   }
   else{
      if (game_over == 1){
        std::cout << "Red wins\n";
      }
      else if (game_over == 2){
         std::cout << "Black wins\n";
      }
      else{
         if (moves_since_take >= max_moves_without_take){
            std::cout << "Draw by 50 move rule\n";
         }
         else{
         std::cout << "Draw by repetition\n";
         }
      }
   }
}

//takes in a Move object, and changes the board state accordingly
void Board::push_move(Move move){
   const uint32_t all = get_all_pieces();
   moves_played++;

   red_bb = move.reds;
   black_bb = move.blacks;
   king_bb = move.kings;

   turn = !turn;
   
   if (king_bb){ //since checkers positions can only be repeated if there are kings, if there are no kings,
      pos_history[hash_bb(red_bb, black_bb, king_bb, turn)] += 1;//        we dont even need to track the position
   }

   moves_since_take = (moves_since_take + 1) * !(move.is_take || move.is_promo);
}

void Board::undo(Move prev_pos, Move curr_pos){
   moves_played--;

   if (king_bb){
      unsigned int hash = hash_bb(red_bb, black_bb, king_bb, turn);
      if (pos_history[hash] == 1){
         pos_history.erase(hash);
      }
      else if (pos_history[hash] > 1) {
         pos_history[hash] -= 1;
      }
   }

   turn = curr_pos.color;
   has_takes = curr_pos.is_take;

   red_bb = prev_pos.reds;
   black_bb = prev_pos.blacks;
   king_bb = prev_pos.kings;
}

uint32_t Board::get_red_movers(){
   const uint32_t empty = ~(red_bb | black_bb);
   uint32_t movers = (empty) >> 4;
   const uint32_t red_kings = red_bb & king_bb;

   movers |= ((empty & MASK_R3) >> 3);
   movers |= ((empty & MASK_R5) >> 5);
   movers &= red_bb;
   if (red_kings){
      movers |= (empty << 4) & red_kings;
      movers |= ((empty & MASK_L3) << 3) & red_kings;
      movers |= ((empty & MASK_L5) << 5) & red_kings;
   }
   return movers;
}

uint32_t Board::get_black_movers(){
   const uint32_t empty = ~(red_bb | black_bb);
   uint32_t movers = (empty) << 4;
   const uint32_t black_kings = black_bb & king_bb;

   movers |= ((empty & MASK_L3) << 3);
   movers |= ((empty & MASK_L5) << 5);
   movers &= black_bb;
   if (black_kings){
      movers |= (empty >> 4) & black_kings;
      movers |= ((empty & MASK_R3) >> 3) & black_kings;
      movers |= ((empty & MASK_R5) >> 5) & black_kings;
   }
   return movers;
}

uint32_t Board::get_red_jumpers(){
   const uint32_t empty = ~(red_bb | black_bb);
   uint32_t jumpers = 0;
   const uint32_t red_kings = red_bb & king_bb;

   uint32_t temp = (empty >> 4) & black_bb;
   jumpers |= (((temp & MASK_R3) >> 3) | ((temp & MASK_R5) >> 5));
   temp = (((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & black_bb;
   jumpers |= (temp >> 4);
   jumpers &= red_bb;

   if (red_kings){
      temp = (empty << 4) & black_bb; // black pieces with empty spaces behind them
      jumpers |= (((temp & MASK_L3) << 3) | ((temp & MASK_L5) << 5)) & red_kings; // gets the squares these pieces can be jumped from+ands it with red kings
      temp = (((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & black_bb; // same process again
      jumpers |= (temp << 4) & red_kings;
   }
   return jumpers;
}

uint32_t Board::get_black_jumpers(){
   const uint32_t empty = ~(red_bb | black_bb);
   uint32_t jumpers = 0;
   const uint32_t black_kings = black_bb & king_bb;

   uint32_t temp = (empty << 4) & red_bb; //red pieces with an open space behind them
   jumpers |= (((temp & MASK_L3) << 3) | ((temp & MASK_L5) << 5)); //the squares that these pieces can be jumped from
   temp = (((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & red_bb; //squares with an open space behind
   jumpers |= (temp << 4); //the squares that these pieces can be jumped from
   jumpers &= black_bb; //Now that we have all the squares that can jump, we and that with the black pieces

   if (black_kings){
      temp = (empty >> 4) & red_bb;
      jumpers |= (((temp & MASK_R3) >> 3) | ((temp & MASK_R5) >> 5)) & black_kings;
      temp = (((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & red_bb;
      jumpers |= (temp >> 4) & black_kings;
   }
   return jumpers;
}

bool Board::add_red_jump(uint32_t jumper, uint32_t temp_red, uint32_t temp_black, uint32_t temp_kings){
   const uint32_t empty = ~(temp_red | temp_black);
   const uint32_t adjusted_red = (temp_red & ~jumper);
   uint32_t taken = (jumper << 4) & temp_black;
   uint32_t dest = (((taken & MASK_L3) << 3) | ((taken & MASK_L5) << 5)) & empty;
   uint32_t new_reds;
   uint32_t new_blacks;
   uint32_t new_kings;
   bool result = false;

   // Does a quick check to see whether this piece can even jump
   uint32_t bb = 0;
   uint32_t temp = (empty >> 4) & temp_black;
   bb |= (((temp & MASK_R3) >> 3) | ((temp & MASK_R5) >> 5));
   temp = (((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & temp_black;
   bb |= (temp >> 4);

   if (jumper & temp_kings){ // king takes
      temp = (empty << 4) & temp_black; // black pieces with empty spaces behind them
      bb |= (((temp & MASK_L3) << 3) | ((temp & MASK_L5) << 5)); // gets the squares these pieces can be jumped from+ands it with red kings
      temp = (((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & temp_black; // same process again
      bb |= (temp << 4);

      if (!(jumper & bb)) return false;

      const uint32_t adjusted_kings = (temp_kings & ~jumper);
      if (taken && dest){
         new_reds = adjusted_red | dest;
         new_blacks = temp_black & ~taken;
         new_kings = (adjusted_kings | dest) & ~taken;
         if (!add_red_jump(dest, new_reds, new_blacks, new_kings)){
            movegen_push(new_reds, new_blacks, new_kings, dest, jumper, 0, false, true);
         }
         result = true;
      }
      taken = (((jumper & MASK_L3) << 3) | ((jumper & MASK_L5) << 5)) & temp_black;
      dest = (taken << 4) & empty;
      if (taken && dest){
         new_reds = adjusted_red | dest;
         new_blacks = temp_black & ~taken;
         new_kings = (adjusted_kings | dest) & ~taken;
         if (!add_red_jump(dest, new_reds, new_blacks, new_kings)){
            movegen_push(new_reds, new_blacks, new_kings, dest, jumper, 0, false, true);
         }
         result = true;
      }
      taken = (jumper >> 4) & temp_black;
      dest = (((taken & MASK_R3) >> 3) | ((taken & MASK_R5) >> 5)) & empty;
      if (taken && dest){
         new_reds = adjusted_red | dest;
         new_blacks = temp_black & ~taken;
         new_kings = (adjusted_kings | dest) & ~taken;
         if (!add_red_jump(dest, new_reds, new_blacks, new_kings)){
            movegen_push(new_reds, new_blacks, new_kings, dest, jumper, 0, false, true);
         }
         result = true;
      }
      taken = (((jumper & MASK_R3) >> 3) | ((jumper & MASK_R5) >> 5)) & temp_black;
      dest = (taken >> 4) & empty;
      if (taken && dest){
         new_reds = adjusted_red | dest;
         new_blacks = temp_black & ~taken;
         new_kings = (adjusted_kings | dest) & ~taken;
         if (!add_red_jump(dest, new_reds, new_blacks, new_kings)){
            movegen_push(new_reds, new_blacks, new_kings, dest, jumper, 0, false, true);
         }
         result = true;
      }
   }
   else{

      if (!(jumper & bb)) return false;

      if (taken && dest){
         new_reds = adjusted_red | dest;
         new_blacks = temp_black & ~taken;
         uint32_t is_promo = (dest & red_promotion_mask);
         new_kings = (is_promo | temp_kings) & ~taken;
         if (is_promo || !add_red_jump(dest, new_reds, new_blacks, new_kings)){
            movegen_push(new_reds, new_blacks, new_kings, dest, jumper, 0, is_promo, true);
         }
         result = true;
      }
      taken = (((jumper & MASK_L3) << 3) | ((jumper & MASK_L5) << 5)) & temp_black;
      dest = (taken << 4) & empty;
      if (taken && dest){
         new_reds = adjusted_red | dest;
         new_blacks = temp_black & ~taken;
         uint32_t is_promo = (dest & red_promotion_mask);
         new_kings = (is_promo | temp_kings) & ~taken;
         if (is_promo || !add_red_jump(dest, new_reds, new_blacks, new_kings)){
            movegen_push(new_reds, new_blacks, new_kings, dest, jumper, 0, is_promo, true);
         }
         result = true;
      }
   }
   return result;
}

bool Board::add_black_jump(uint32_t jumper, uint32_t temp_red, uint32_t temp_black, uint32_t temp_kings){
   const uint32_t empty = ~(temp_red | temp_black);
   const uint32_t adjusted_black = (temp_black & ~jumper);
   uint32_t taken = (jumper >> 4) & temp_red;
   uint32_t dest = (((taken & MASK_R3) >> 3) | ((taken & MASK_R5) >> 5)) & empty;
   uint32_t new_reds;
   uint32_t new_blacks;
   uint32_t new_kings;
   bool result = false;

   uint32_t bb = 0;
   uint32_t temp = (empty << 4) & temp_red;
   bb |= (((temp & MASK_L3) << 3) | ((temp & MASK_L5) << 5));
   temp = (((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & temp_red;
   bb |= (temp << 4);

   if (jumper & temp_kings){

      temp = (empty >> 4) & temp_red;
      bb |= (((temp & MASK_R3) >> 3) | ((temp & MASK_R5) >> 5));
      temp = (((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & temp_red;
      bb |= (temp >> 4);

      if (!(jumper & bb)) return false;
      
      const uint32_t adjusted_kings = (temp_kings & ~jumper);
      if (taken && dest){
         new_reds = temp_red & ~taken;
         new_blacks = adjusted_black | dest;
         new_kings = (adjusted_kings | dest) & ~taken;
         if (!add_black_jump(dest, new_reds, new_blacks, new_kings)){
            movegen_push(new_reds, new_blacks, new_kings, dest, jumper, 1, false, true);
         }
         result = true;
      }
      taken = (((jumper & MASK_R3) >> 3) | ((jumper & MASK_R5) >> 5)) & temp_red;
      dest = (taken >> 4) & empty;
      if (taken && dest){
         new_reds = temp_red & ~taken;
         new_blacks = adjusted_black | dest;
         new_kings = (adjusted_kings | dest) & ~taken;
         if (!add_black_jump(dest, new_reds, new_blacks, new_kings)){
            movegen_push(new_reds, new_blacks, new_kings, dest, jumper, 1, false, true);
         }
         result = true;
      }

      taken = (jumper << 4) & temp_red;
      dest = (((taken & MASK_L3) << 3) | ((taken & MASK_L5) << 5)) & empty;
      if (taken && dest){
         new_reds = temp_red & ~taken;
         new_blacks = adjusted_black | dest;
         new_kings = (adjusted_kings | dest) & ~taken;
         if (!add_black_jump(dest, new_reds, new_blacks, new_kings)){
            movegen_push(new_reds, new_blacks, new_kings, dest, jumper, 1, false, true);
         }
         result = true;
      }
      taken = (((jumper & MASK_L3) << 3) | ((jumper & MASK_L5) << 5)) & temp_red;
      dest = (taken << 4) & empty;
      if (taken && dest){
         new_reds = temp_red & ~taken;
         new_blacks = adjusted_black | dest;
         new_kings = (adjusted_kings | dest) & ~taken;
         if (!add_black_jump(dest, new_reds, new_blacks, new_kings)){
            movegen_push(new_reds, new_blacks, new_kings, dest, jumper, 1, false, true);
         }
         result = true;
      }
   }
   else{
      if (!(jumper & bb)) return false;

      if (taken && dest){
         new_reds = temp_red & ~taken;
         new_blacks = adjusted_black | dest;
         uint32_t is_promo = dest & black_promotion_mask;
         new_kings = (is_promo | temp_kings) & ~taken;
         if (is_promo || !add_black_jump(dest, new_reds, new_blacks, new_kings)){
            movegen_push(new_reds, new_blacks, new_kings, dest, jumper, 1, is_promo, true);
         }
         result = true;
      }

      taken = (((jumper & MASK_R3) >> 3) | ((jumper & MASK_R5) >> 5)) & temp_red;
      dest = (taken >> 4) & empty;
      if (taken && dest){
         new_reds = temp_red & ~taken;
         new_blacks = adjusted_black | dest;
         uint32_t is_promo = dest & black_promotion_mask;
         new_kings = (is_promo | temp_kings) & ~taken;
         if (is_promo || !add_black_jump(dest, new_reds, new_blacks, new_kings)){
            movegen_push(new_reds, new_blacks, new_kings, dest, jumper, 1, is_promo, true);
         }
         result = true;
      }
   }
   return result;
}
//generates all legal moves
int Board::gen_moves(Move * moves){
   m = moves;
   movecount = 0;
   has_takes = false;
   const uint32_t empty = ~(red_bb | black_bb);

   //if turn == 1 (black's turn), generate moves for black
   if (turn){
      uint32_t jumpers = get_black_jumpers();

      if (jumpers){
         has_takes = true;
         uint32_t new_reds;
         uint32_t new_blacks;
         uint32_t new_kings;
         while (jumpers){
            const uint32_t piece = jumpers & -jumpers;
            const uint32_t adjusted_black = (black_bb & ~piece);

            uint32_t taken = (piece >> 4) & red_bb;
            uint32_t dest = (((taken & MASK_R3) >> 3) | ((taken & MASK_R5) >> 5)) & empty;
            if (piece & king_bb){ //King takes
               const uint32_t adjusted_kings = (king_bb & ~piece);
               if (taken && dest){
                  new_reds = red_bb & ~taken;
                  new_blacks = adjusted_black | dest;
                  new_kings = (adjusted_kings | dest) & ~taken;
                  if (!add_black_jump(dest, new_reds, new_blacks, new_kings)){
                     movegen_push(new_reds, new_blacks, new_kings, dest, piece, 1, false, true);
                  }
               }
               taken = (((piece & MASK_R3) >> 3) | ((piece & MASK_R5) >> 5)) & red_bb;
               dest = (taken >> 4) & empty;
               if (taken && dest){
                  new_reds = red_bb & ~taken;
                  new_blacks = adjusted_black | dest;
                  new_kings = (adjusted_kings | dest) & ~taken;
                  if (!add_black_jump(dest, new_reds, new_blacks, new_kings)){
                     movegen_push(new_reds, new_blacks, new_kings, dest, piece, 1, false, true);
                  }
               }

               taken = (piece << 4) & red_bb;
               dest = (((taken & MASK_L3) << 3) | ((taken & MASK_L5) << 5)) & empty;
               if (taken && dest){
                  new_reds = red_bb & ~taken;
                  new_blacks = adjusted_black | dest;
                  new_kings = (adjusted_kings | dest) & ~taken;
                  if (!add_black_jump(dest, new_reds, new_blacks, new_kings)){
                     movegen_push(new_reds, new_blacks, new_kings, dest, piece, 1, false, true);
                  }
               }
               taken = (((piece & MASK_L3) << 3) | ((piece & MASK_L5) << 5)) & red_bb;
               dest = (taken << 4) & empty;
               if (taken && dest){
                  new_reds = red_bb & ~taken;
                  new_blacks = adjusted_black | dest;
                  new_kings = (adjusted_kings | dest) & ~taken;
                  if (!add_black_jump(dest, new_reds, new_blacks, new_kings)){
                     movegen_push(new_reds, new_blacks, new_kings, dest, piece, 1, false, true);
                  }
               }
            }
            else{ //regular takes
               if (taken && dest){
                  new_reds = red_bb & ~taken;
                  new_blacks = adjusted_black | dest;
                  uint32_t is_promo = dest & black_promotion_mask;
                  new_kings = (is_promo | king_bb) & ~taken;
                  if (is_promo || !add_black_jump(dest, new_reds, new_blacks, new_kings)){
                     movegen_push(new_reds, new_blacks, new_kings, dest, piece, 1, is_promo, true);
                  }
               }
   
               taken = (((piece & MASK_R3) >> 3) | ((piece & MASK_R5) >> 5)) & red_bb;
               dest = (taken >> 4) & empty;
               if (taken && dest){
                  new_reds = red_bb & ~taken;
                  new_blacks = adjusted_black | dest;
                  uint32_t is_promo = dest & black_promotion_mask;
                  new_kings = (is_promo | king_bb) & ~taken;
                  if (is_promo || !add_black_jump(dest, new_reds, new_blacks, new_kings)){
                     movegen_push(new_reds, new_blacks, new_kings, dest, piece, 1, is_promo, true);
                  }
               }
            }
            jumpers &= jumpers-1;
         }
      }
      else{    //Regular moves
         uint32_t movers = get_black_movers();

         while (movers){
            const uint32_t piece = movers & -movers;
            uint32_t dest = (piece >> 4) & empty;

            if (piece & king_bb){ // king moves
               if (dest){
                  movegen_push(red_bb, (black_bb & ~piece) | dest, 
                                        (king_bb & ~piece) | dest, dest, piece, 1, false, false);
               }
               dest = (((piece & MASK_R3) >> 3) | ((piece & MASK_R5) >> 5)) & empty;
               if (dest){
                  movegen_push(red_bb, (black_bb & ~piece) | dest, 
                                          (king_bb & ~piece) | dest, dest, piece, 1, false, false);
               }
               dest = (piece << 4) & empty;
               if (dest){
                  movegen_push(red_bb, (black_bb & ~piece) | dest, 
                                          (king_bb & ~piece) | dest, dest, piece, 1, false, false);
               }
               dest = (((piece & MASK_L3) << 3) | ((piece & MASK_L5) << 5)) & empty;
               if (dest){
                  movegen_push(red_bb, (black_bb & ~piece) | dest, 
                                          (king_bb & ~piece) | dest, dest, piece, 1, false, false);
               }
            }
            else{// piece moves
               if (dest){
                  movegen_push(red_bb, (black_bb & ~piece) | dest, 
                                             (dest & black_promotion_mask) | king_bb, dest, piece, 1, (dest & black_promotion_mask), false);
               }
               dest = (((piece & MASK_R3) >> 3) | ((piece & MASK_R5) >> 5)) & empty;
               if (dest){
                  movegen_push(red_bb, (black_bb & ~piece) | dest, 
                                             (dest & black_promotion_mask) | king_bb, dest, piece, 1, (dest & black_promotion_mask), false);
               }
            }
            movers &= movers-1;
         }
      }
   }
   else{
      uint32_t jumpers = get_red_jumpers();
      if (jumpers){
         has_takes = true;
         uint32_t new_reds;
         uint32_t new_blacks;
         uint32_t new_kings;

         while (jumpers){ // takes
            const uint32_t piece = jumpers & -jumpers;
            const uint32_t adjusted_red = (red_bb & ~piece);

            uint32_t taken = (piece << 4) & black_bb;
            uint32_t dest = (((taken & MASK_L3) << 3) | ((taken & MASK_L5) << 5)) & empty;

            if (piece & king_bb){ // king takes
               const uint32_t adjusted_kings = (king_bb & ~piece);

               if (taken && dest){
                  new_reds = adjusted_red | dest;
                  new_blacks = black_bb & ~taken;
                  new_kings = (adjusted_kings | dest) & ~taken;
                  if (!add_red_jump(dest, new_reds, new_blacks, new_kings)){
                     movegen_push(new_reds, new_blacks, new_kings, dest, piece, 0, false, true);
                  }
               }

               taken = (((piece & MASK_L3) << 3) | ((piece & MASK_L5) << 5)) & black_bb;
               dest = (taken << 4) & empty;
               if (taken && dest){
                  new_reds = adjusted_red | dest;
                  new_blacks = black_bb & ~taken;
                  new_kings = (adjusted_kings | dest) & ~taken;
                  if (!add_red_jump(dest, new_reds, new_blacks, new_kings)){
                     movegen_push(new_reds, new_blacks, new_kings, dest, piece, 0, false, true);
                  }
               }

               taken = (piece >> 4) & black_bb;
               dest = (((taken & MASK_R3) >> 3) | ((taken & MASK_R5) >> 5)) & empty;
               if (taken && dest){
                  new_reds = adjusted_red | dest;
                  new_blacks = black_bb & ~taken;
                  new_kings = (adjusted_kings | dest) & ~taken;
                  if (!add_red_jump(dest, new_reds, new_blacks, new_kings)){
                     movegen_push(new_reds, new_blacks, new_kings, dest, piece, 0, false, true);
                  }
               }

               taken = (((piece & MASK_R3) >> 3) | ((piece & MASK_R5) >> 5)) & black_bb;
               dest = (taken >> 4) & empty;
               if (taken && dest){
                  new_reds = adjusted_red | dest;
                  new_blacks = black_bb & ~taken;
                  new_kings = (adjusted_kings | dest) & ~taken;
                  if (!add_red_jump(dest, new_reds, new_blacks, new_kings)){
                     movegen_push(new_reds, new_blacks, new_kings, dest, piece, 0, false, true);
                  }
               }
            }
            else{// piece takes
               if (taken && dest){
                  new_reds = adjusted_red | dest;
                  new_blacks = black_bb & ~taken;
                  uint32_t is_promo = (dest & red_promotion_mask);
                  new_kings = (is_promo | king_bb) & ~taken;
                  if (is_promo || !add_red_jump(dest, new_reds, new_blacks, new_kings)){
                     movegen_push(new_reds, new_blacks, new_kings, dest, piece, 0, is_promo, true);
                  }
               }

               taken = (((piece & MASK_L3) << 3) | ((piece & MASK_L5) << 5)) & black_bb;
               dest = (taken << 4) & empty;
               if (taken && dest){
                  new_reds = adjusted_red | dest;
                  new_blacks = black_bb & ~taken;
                  uint32_t is_promo = (dest & red_promotion_mask);
                  new_kings = (is_promo | king_bb) & ~taken;
                  if (is_promo || !add_red_jump(dest, new_reds, new_blacks, new_kings)){
                     movegen_push(new_reds, new_blacks, new_kings, dest, piece, 0, is_promo, true);
                  }
               }
            }
            jumpers &= jumpers-1;
         }
      }
      else{ //    Regular moves
         uint32_t movers = get_red_movers();

         while(movers){
            const uint32_t piece = movers & -movers;
            uint32_t dest = (piece << 4) & empty;
            if (piece & king_bb){ //   king moves
               if (dest){
                  movegen_push((red_bb & ~piece) | dest, black_bb, 
                                             (king_bb & ~piece) | dest, dest, piece, 0, false, false);
               }
               dest = (((piece & MASK_L3) << 3) | ((piece & MASK_L5) << 5)) & empty;
               if (dest){
                  movegen_push((red_bb & ~piece) | dest, black_bb, 
                                             (king_bb & ~piece) | dest, dest, piece, 0, false, false);
               }
               dest = (piece >> 4) & empty;
               if (dest){
                  movegen_push((red_bb & ~piece) | dest, black_bb, 
                                          (king_bb & ~piece) | dest, dest, piece, 0, false, false);
               }
               dest = (((piece & MASK_R3) >> 3) | ((piece & MASK_R5) >> 5)) & empty;
               if (dest){
                  movegen_push((red_bb & ~piece) | dest, black_bb, 
                                          (king_bb & ~piece) | dest, dest, piece, 0, false, false);
               }
            }
            else{ // regular moves
               if (dest){
               movegen_push((red_bb & ~piece) | dest, black_bb, 
                                          (dest & red_promotion_mask) | king_bb, dest, piece, 0, (dest & red_promotion_mask), false);
               }
               dest = (((piece & MASK_L3) << 3) | ((piece & MASK_L5) << 5)) & empty;
               if (dest){
                  movegen_push((red_bb & ~piece) | dest, black_bb, 
                                             (dest & red_promotion_mask) | king_bb, dest, piece, 0, (dest & red_promotion_mask), false);
               }
            }
            movers &= movers-1;
         }
      }
   }
   return movecount;
}

int Board::get_tempo_score(uint32_t piece, int color){
   if (color){
      if (piece & ROW5) return 100;
      if (piece & ROW4) return 200;
      if (piece & ROW3) return 300;
      if (piece & ROW2) return 400;
      if (piece & ROW1) return 500;
      return 0;
   }
   else{
      if (piece & ROW4) return 100;
      if (piece & ROW5) return 200;
      if (piece & ROW6) return 300;
      if (piece & ROW7) return 400;
      if (piece & ROW8) return 500;
      return 0;
   }
}
void Board::movegen_push(uint32_t new_reds, uint32_t new_blacks, uint32_t new_kings, uint32_t to, uint32_t from, int color, bool is_promo, bool is_take){
   m[movecount].reds = new_reds;
   m[movecount].blacks = new_blacks;
   m[movecount].kings = new_kings;
   m[movecount].to = to;
   m[movecount].from = from;
   m[movecount].color = color;
   m[movecount].is_promo = is_promo;
   m[movecount].is_take = is_take;
   m[movecount].score = 0;

   if (is_promo) m[movecount].score += PROMO_SORT;
   if (!(to & new_kings)) m[movecount].score += get_tempo_score(to, color);

   movecount++;
}

bool Board::can_jump(uint32_t piece, int color){
   if (color){
      return get_black_jumpers() & piece;
   }
   return get_red_jumpers() & piece;
}

//returns a random legal move
//NOTE: I'm not entirely sure if this works its a little iffy for some reason
Move Board::get_random_move(){
   srand(time(NULL));
   int index = rand() % movecount;
   return m[movecount];
}

//sets the board to a random position by playing a "moves_to_play" random moves
void Board::set_random_pos(int moves_to_play){
   for (int i = 0; i < moves_to_play; i++){
      push_move(get_random_move());
   }
}

/*Returns an int that represents the result.
If the game is not over, 0 is returned.
A red win returns 1, a black win returns 2.
Use check_repetition for checking draws.*/
int Board::check_win(){
   if (movecount == 0){
      if (!red_bb){ //if red has no pieces, black wins
         return 2;
      }
      if (!black_bb){ //if black has no pieces, red wins
         return 1;
      }
      /*If both sides still have pieces, but there are no legal moves,
      then the side whos turn it is to move loses */
      if (turn){
         return 1;
      }
      else{
         return 2;
      }
   }
   return 0;//otherwise, the game is not over
}

/*
Returns 1 if the game is a draw by repetition
or 50 move rule (these are the only kinds of draws
in checkers).
*/
int Board::check_repetition(){
   if ((king_bb && (pos_history[hash_bb(red_bb, black_bb, king_bb, turn)] >= repetition_limit)) || (moves_since_take >= max_moves_without_take)){
      return 1;
   }
   return 0;
}

uint32_t Move::get_end_square(const uint32_t previous_pos) const{
   return (reds | blacks) & ~previous_pos;
}

//prints the start and end square of the move, as well as the middle (if applicable)
void Move::get_move_info(uint32_t previous_pos) const{
   uint32_t start = previous_pos & ~(reds | blacks);
   uint32_t middle = 0;
   uint32_t end = (reds | blacks) & ~previous_pos;

   if ((start & start-1) != 0){
      long long x = start & -start;
      if (abs(end - x) < (end - (start & start-1))){
         middle = x;
         start &= start-1;
      }
      else{
         middle = start & start-1;
         start = x;
      }
      if (middle > start && middle > end){
         start ^= middle;
         middle ^= start;
         start ^= middle;
      }
   }

   if (middle){
      std::cout << binary_to_square(start) << "-" << binary_to_square(middle) << "-" << binary_to_square(end);
   }
   else{
      std::cout << binary_to_square(start) << "-" << binary_to_square(end);
   }
   // std::cout << "Start: expected square is " << binary_to_square(start) << ", binary is: ";
   // print_binary(start);

   // if (middle){
   //    std::cout << "Taken: expected square is " << binary_to_square(middle) << ", binary is: ";
   //    print_binary(middle);
   // }

   // std::cout << "End: expected square is " << binary_to_square(end) << ", binary is: ";
   // print_binary(end);

   // std::array<int, 2> result = {binary_to_square(start), binary_to_square(end)};
   // std::cout << "\n";
}