#ifndef MISC
#define MISC

#include <math.h>
#include <vector>
#include <iostream>
#include <bitset>
#include <chrono>
#include <nmmintrin.h>

//returns the nubmer of 1 bits in a bitboard
//aka the "Hamming Weight"
inline int count_bits(uint32_t bb)
{
   #ifdef __GNUC__	
	   return __builtin_popcountll(bb);
   #else
	   return _mm_popcnt_u32(bb);
   #endif
}

//gets the square index of a binary number
//NOTE: Returns the square - 1, so just be aware
inline int binary_to_square(const uint32_t binary){
   if (binary == 0) return 0;
   return __builtin_ctzll(binary);
}


//hashes the bitboard
inline uint64_t hash_bb(uint32_t reds, uint32_t blacks, uint32_t kings, int turn){
   return ((reds * 37 + blacks) * 37 + kings) * 37 + turn;
}

inline uint32_t northFill(uint32_t bb){
   bb |= (bb << 8);
   bb |= (bb << 16);
   return bb;
}

inline uint32_t southFill(uint32_t bb){
   bb |= (bb >> 8);
   bb |= (bb >> 16);
   return bb;
}

uint32_t square_to_binary(const int square);
std::vector<int> serialize_bb(uint32_t bb);
void print_binary(uint32_t num);
double get_time();
#endif