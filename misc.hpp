#ifndef MISC
#define MISC

#include <math.h>
#include <vector>
#include <iostream>
#include <bitset>
#include <chrono>
#include <nmmintrin.h>

int count_bits(uint32_t bb);
uint32_t square_to_binary(const int square);
int binary_to_square(const uint32_t binary);
std::vector<int> serialize_bb(uint32_t bb);
void print_binary(uint32_t num);
double get_time();

//hashes the bitboard
inline uint64_t hash_bb(uint32_t reds, uint32_t blacks, uint32_t kings, int turn){
   const unsigned int hash = ((reds * 37 + blacks) * 37 + kings) * 37 + turn;
   return hash;
}

#endif