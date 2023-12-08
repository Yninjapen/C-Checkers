#include <iostream>
#include <bitset>

//prints a binary representation of a number
void print_binary(long long num){
   std::cout << std::bitset<32>(num) << "\n";
}

int main(){
    long long a = 0b101000000000;
    long long b = 0b000001100000;
    long long mask = 0b111110111111;
    a = (a ^ b) & mask;
    b ^= a;
    a ^= b;

    print_binary(a);
    print_binary(b);
}