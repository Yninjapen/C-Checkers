#include <iostream>
#include <bitset>
#include <algorithm>
#include <math.h>
//prints a binary representation of a number
void print_binary(long long num){
   std::cout << std::bitset<32>(num) << "\n";
}

int main(){
    long long a = 0b1111;
    long long b = a;
    b &=b-1;
    print_binary(a);
    print_binary(b);
}