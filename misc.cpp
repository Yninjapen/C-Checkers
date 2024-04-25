#include "misc.hpp"

//returns the nubmer of 1 bits in a bitboard
//aka the "Hamming Weight"
int count_bits(uint32_t bb)
{
   #ifdef __GNUC__	
	   return __builtin_popcountll(bb);
   #else
	   return _mm_popcnt_u32(bb);
   #endif
}

//converts a square index (according to the chart above) to its bitboard representation
uint32_t square_to_binary(const int square){
   uint32_t n = 0b1;
   n = n << (square - 1);
   return n;
}

//gets the square index of a binary number
//NOTE: THERE CAN ONLY BE ONE 1 BIT
int binary_to_square(const uint32_t binary){
   return log2(binary) + 1;
}

//gets the squares of all 1 bits in a bitboard
std::vector<int> serialize_bb(uint32_t bb){
   std::vector<int> squares;
   if(bb == 0){return squares;}

   while (bb){
      uint32_t ls1b = bb & -bb; // isolate LS1B
      squares.push_back(binary_to_square(ls1b)); //add squares to vector
      bb &= bb-1; // reset LS1B
   }
   return squares;
}

//prints a binary representation of a number
void print_binary(uint32_t num){
   std::cout << std::bitset<32>(num) << "\n";
}

//hashes the bitboard
unsigned int hash_bb(uint32_t reds, uint32_t blacks, uint32_t kings, int turn){
   const unsigned int hash = ((reds * 37 + blacks) * 37 + kings) * 37 + turn;
   return hash;
}

//returns time in milliseconds
double get_time(){
   return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}