#include <iostream>
#include <bitset>
#include <algorithm>
#include <math.h>
//prints a binary representation of a number
void print_binary(long long num){
   std::cout << std::bitset<128>(num) << "\n";
}

int main(){
    __uint128_t a = 0b1111;
    __uint128_t b = 0b0111;
    __uint128_t c = 0b0011;
    __uint128_t d = 0b0001;
    print_binary((((((d << 32) | c) << 32) | b) << 32) | a);
}

// 3 3 5 6 4 5 7 0 1 2 0 5 0 4 0 0 1 0 1 0 0 7 4 6 2 2 0 0 0 0 1 0 0 8 0 7 0 10 0 7 1 9 6 7 1 8 8 4 2 1 1 0 6 0 6 0 0 11 0 7 0 9 0 0 0 8