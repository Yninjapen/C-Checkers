#ifndef MISC
#define MISC

#include <math.h>
#include <vector>
#include <iostream>
#include <bitset>
#include <chrono>

int count_bits(uint32_t bb);
uint32_t square_to_binary(const int square);
int binary_to_square(const uint32_t binary);
std::vector<int> serialize_bb(uint32_t bb);
void print_binary(uint32_t num);
unsigned int hash_bb(uint32_t reds, uint32_t blacks, uint32_t kings, int turn);
double get_time();
#endif