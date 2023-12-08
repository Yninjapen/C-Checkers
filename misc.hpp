#ifndef MISC
#define MISC

#include <math.h>
#include <vector>
#include <iostream>
#include <bitset>

int count_bits(long long bb);
long long square_to_binary(const int square);
int binary_to_square(const long long binary);
std::vector<int> serialize_bb(long long bb);
void print_binary(long long num);

#endif