#include "misc.hpp"

//converts a square index (according to the chart above) to its bitboard representation
uint32_t square_to_binary(const int square){
   return 1 << square;
}

//gets the squares of all 1 bits in a bitboard
std::vector<int> serialize_bb(uint32_t bb){
   std::vector<int> squares;
   if(bb == 0){return squares;}

   while (bb){
      uint32_t ls1b = bb & -bb; // isolate LS1B
      squares.push_back(binary_to_square(ls1b) + 1); //add squares to vector
      bb &= bb-1; // reset LS1B
   }
   return squares;
}

//prints a binary representation of a number
void print_binary(uint32_t num){
   std::cout << std::bitset<32>(num) << "\n";
}

//returns time in milliseconds
uint64_t get_time(){
   return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}