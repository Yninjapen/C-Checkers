#include <iostream>
#include <array>
#include <bitset>
#include <random>
#include "board.hpp"
#include "move_tables.hpp"

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
   legal_moves = gen_moves();

   Move initial_pos(4095, 4293918720, 0, 1);
   move_history.push_back(initial_pos);

   game_over = 0;
}

unsigned long long Board::get_all_pieces(){
   return red_bb | black_bb;
}

//converts a square index (according to the chart above) to its bitboard representation
long long square_to_binary(const int square){
   long long n = 0b1;
   n = n << (square - 1);
   return n;
}

//gets the square index of a binary number
//NOTE: THERE CAN ONLY BE ONE 1 BIT
int binary_to_square(const long long binary){
   return log2(binary) + 1;
}

//gets the squares of all 1 bits in a bitboard
std::vector<int> serialize_bb(long long bb){
   std::vector<int> squares;
   if(bb == 0){return squares;}

   while (bb){
      long long ls1b = bb & -bb; // isolate LS1B
      squares.push_back(binary_to_square(ls1b)); //add squares to vector
      bb &= bb-1; // reset LS1B
   }
   return squares;
}

//prints a binary representation of a number
void print_binary(long long num){
   std::cout << std::bitset<32>(num) << "\n";
}

//prints a representation of the board to the console
void Board::print_board(){
   char arr[32] = {' '};
   long long red_kings = red_bb & king_bb;
   long long red_pieces = red_bb & ~king_bb;
   long long black_kings = black_bb & king_bb;
   long long black_pieces = black_bb & ~king_bb;
   std::vector<int> v;

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
   }
   std::cout << "\n";
}

//takes in a Move object, and changes the board state accordingly
void Board::push_move(Move move){
   const long long all = get_all_pieces();

   red_bb = move.reds;
   black_bb = move.blacks;
   king_bb = move.kings;

   has_takes = false;
   if (move.is_take && can_jump(move.get_end_square(all), turn)){
      has_takes = true;
      turn = !turn;
   }


   turn = !turn;
   legal_moves = gen_moves();
   game_over = is_game_over();
   move_history.push_back(move);
}

void Board::undo(){
   turn = move_history[move_history.size() - 1].color;
   move_history.pop_back();
   const Move move = move_history[move_history.size() - 1];
   
   red_bb = move.reds;
   black_bb = move.blacks;
   king_bb = move.kings;

   has_takes = false;
   if (move.is_take){
      has_takes = true;
   }

   legal_moves = gen_moves();
}

//generates all legal moves
std::vector<Move> Board::gen_moves(){
   const unsigned long long empty = ~(red_bb | black_bb);
   std::vector<Move> moves;
   std::vector<Move> takes;

   //if turn == 1 (black's turn), generate moves for black
   if (turn){
      const unsigned long long black_pieces = black_bb & ~king_bb;
      const unsigned long long black_kings = black_bb & king_bb;
      const std::vector<int> black_locations = serialize_bb(black_pieces);
      const std::vector<int> king_locations = serialize_bb(black_kings);
      unsigned long long bb;
      long long t;
      long long o;
      long long mask;
      int row;

      //black piece moves
      for (int i = 0; i < black_locations.size(); i++){

         //black piece takes
         t = black_takes[black_locations[i]] & empty;
         o = black_moves[black_locations[i]] & red_bb;
         if (t && o){
            row = ceil((double)black_locations[i]/4);
            int shift = 4 - (row%2);
            while(o){
               long long ls1b = o & -o;
               mask = ((ls1b | (ls1b >> 1)) >> shift) & t;
               if (mask){
                  takes.push_back(Move(red_bb & ~ls1b, (black_bb & ~square_to_binary(black_locations[i])) | (mask & -mask), (mask & black_promotion_mask) | king_bb & ~ls1b, 1, true));
                  has_takes = true;
               }
               o &= o-1;
            }
         }

         //black piece non-takes
         if (!has_takes){}
            bb = black_moves[black_locations[i]] & empty;
            while (bb){
               long long ls1b = bb & -bb;
               moves.push_back(Move(red_bb, (black_bb & ~square_to_binary(black_locations[i])) | ls1b, (ls1b & black_promotion_mask) | king_bb, 1));
               bb &= bb-1;
            }
      }

      //black king moves
      for (int i = 0; i < king_locations.size(); i++){
         const unsigned long long inverse_location = ~square_to_binary(king_locations[i]);

         //black king south takes
         t = black_takes[king_locations[i]] & empty;
         o = black_moves[king_locations[i]] & red_bb;
         if (t && o){
            row = ceil((double)king_locations[i]/4);
            int shift = 4 - (row%2);
            while(o){
               long long ls1b = o & -o;
               mask = ((ls1b | (ls1b >> 1)) >> shift) & t;
               if (mask){
                  takes.push_back(Move(red_bb & ~ls1b, (black_bb & inverse_location) | (mask & -mask), ((king_bb & inverse_location) | (mask & -mask)) | king_bb & ~ls1b, 1, true));
                  has_takes = true;
               }
               o &= o-1;
            }
         }

         //black king north takes
         t = red_takes[king_locations[i]] & empty;
         o = red_moves[king_locations[i]] & red_bb;
         if (t && o){
            row = ceil((double)king_locations[i]/4);
            int shift = (row%2) + 3;
            while(o){
               long long ls1b = o & -o;
               mask = ((ls1b | (ls1b << 1)) << shift) & t;
               if (mask){
                  takes.push_back(Move(red_bb & ~ls1b, (black_bb & inverse_location) | (mask & -mask), ((king_bb & inverse_location) | (mask & -mask)) | king_bb & ~ls1b, 1, true));
                  has_takes = true;
               }
               o &= o-1;
            }
         }

         //black king non-takes
         if (!has_takes){
            bb = king_moves[king_locations[i]] & empty;
            while (bb){
               long long ls1b = bb & -bb;
               moves.push_back(Move(red_bb, (black_bb & inverse_location) | ls1b, (king_bb & inverse_location) | ls1b, 1));
               bb &= bb-1;
            }
         }
      }
   }

   //Otherwise, generate moves for red
   else{
      const unsigned long long red_pieces = red_bb & ~king_bb;
      const unsigned long long red_kings = red_bb & king_bb;
      const std::vector<int> red_locations = serialize_bb(red_pieces);
      const std::vector<int> king_locations = serialize_bb(red_kings);
      unsigned long long bb;
      long long t;
      long long o;
      long long mask;
      int row;

      for (int i = 0; i < red_locations.size(); i++){

         //red piece takes
         t = red_takes[red_locations[i]] & empty;
         o = red_moves[red_locations[i]] & black_bb;
         if (t && o){
            row = ceil((double)red_locations[i]/4);
            int shift = (row%2) + 3;
            while(o){
               long long ls1b = o & -o;
               mask = ((ls1b | (ls1b << 1)) << shift) & t;
               if (mask){
                  takes.push_back(Move((red_bb & ~square_to_binary(red_locations[i])) | (mask & -mask), black_bb & ~ls1b, (mask & red_promotion_mask) | king_bb & ~ls1b, 0, true));
                  has_takes = true;
               }
               o &= o-1;
            }
         }

         //red piece non-takes
         if (!has_takes){
            bb = red_moves[red_locations[i]] & empty;
            while (bb){
               long long ls1b = bb & -bb;
               moves.push_back(Move((red_bb & ~square_to_binary(red_locations[i])) | ls1b, black_bb, (ls1b & red_promotion_mask) | king_bb, 0));
               bb &= bb-1;
            }
         }
      }

      for (int i = 0; i < king_locations.size(); i++){
         const unsigned long long inverse_location = ~square_to_binary(king_locations[i]);

         //red king north takes
         t = red_takes[king_locations[i]] & empty;
         o = red_moves[king_locations[i]] & black_bb;
         if (t && o){
            row = ceil((double)king_locations[i]/4);
            int shift = (row%2) + 3;
            while(o){
               long long ls1b = o & -o;
               mask = ((ls1b | (ls1b << 1)) << shift) & t;
               if (mask){
                  takes.push_back(Move((red_bb & inverse_location) | (mask & -mask), black_bb & ~ls1b, ((king_bb & inverse_location) | (mask & -mask)) | king_bb & ~ls1b, 0, true));
                  has_takes = true;
               }
               o &= o-1;
            }
         }

         //red king south takes
         t = black_takes[king_locations[i]] & empty;
         o = black_moves[king_locations[i]] & black_bb;
         if (t && o){
            row = ceil((double)king_locations[i]/4);
            int shift = 4 - (row%2);
            while(o){
               long long ls1b = o & -o;
               mask = ((ls1b | (ls1b >> 1)) >> shift) & t;
               if (mask){
                  takes.push_back(Move((red_bb & inverse_location) | (mask & -mask), black_bb & ~ls1b, ((king_bb & inverse_location) | (mask & -mask)) | king_bb & ~ls1b, 0, true));
                  has_takes = true;
               }
               o &= o-1;
            }
         }

         //red king non-takes
         if (!has_takes){
            bb = king_moves[king_locations[i]] & empty;
            while (bb){
               long long ls1b = bb & -bb;
               moves.push_back(Move((red_bb & inverse_location) | ls1b, black_bb, (king_bb & inverse_location) | ls1b, 0));
               bb &= bb-1;
            }
         }
      }
   }

   //if there are takes, return them instead of the regular moves
   if (has_takes){
      // std::cout << "printing " << takes.size() << " takes:\n";
      // for (int i = 0; i < takes.size(); i++){takes[i].get_move_info(get_all_pieces());}
      // std::cout << "done printing takes \n";
      return takes;
   }
   return moves;
}

bool Board::can_jump(long long piece, int color){
   const long long empty = ~(red_bb | black_bb);
   const int square = binary_to_square(piece);
   long long t;
   long long o;
   long long mask;
   
   if (color){
      //black piece takes
      t = black_takes[square] & empty;
      o = black_moves[square] & red_bb;
      if (t && o){
         int row = ceil((double)square/4);
         int shift = 4 - (row%2);
         mask = ((o | (o >> 1)) >> shift) & t;
         if (mask){
            return true;
         }
      }
      //black king takes
      if (piece & king_bb){
         t = red_takes[square] & empty;
         o = red_moves[square] & red_bb;
         if (t && o){
            int row = ceil((double)square/4);
            int shift = (row%2) + 3;
            mask = ((o | (o << 1)) << shift) & t;
            if (mask){
               return true;
            }
         }
      }
   }
   else{
      //red piece takes
      t = red_takes[square] & empty;
      o = red_moves[square] & black_bb;
      if (t && o){
         int row = ceil((double)square/4);
         int shift = (row%2) + 3;
         mask = ((o | (o << 1)) << shift) & t;
         if (mask){
            return true;
         }
      }
      //red king takes
      if (piece & king_bb){
         t = black_takes[square] & empty;
         o = black_moves[square] & black_bb;
         if (t && o){
            int row = ceil((double)square/4);
            int shift = 4 - (row%2);
            mask = ((o | (o >> 1)) >> shift) & t;
            if (mask){
               return true;
            }
         }
      }
   }
   return false;
}

Move Board::get_random_move(){
   int index = rand() % legal_moves.size();
   return legal_moves[index];
}

/*Returns an int that represents the result.
If the game is not over, 0 is returned.
A red win returns 1, a black win returns 2,
and a draw returns -1*/
int Board::is_game_over(){
   if (legal_moves.size() == 0){
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

//constructor for the move class
Move::Move(long long r, long long b, long long k, int c, bool take){
   reds = r;
   blacks = b;
   kings = k;
   color = c;
   is_take = take;
}

long long Move::get_end_square(const unsigned long long previous_pos){
   return (reds | blacks) & ~previous_pos;
}

//returns the start and end squares of the move as an array
void Move::get_move_info(unsigned long long previous_pos){
   long long start = previous_pos & ~(reds | blacks);
   long long middle = 0;
   long long end = (reds | blacks) & ~previous_pos;

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