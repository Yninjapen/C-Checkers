#include <iostream>
#include <bitset>

//prints a binary representation of a number
void print_binary(long long num){
   std::cout << std::bitset<32>(num) << "\n";
}

int main(){
    int a = 0;
    a++;
    int b = 5;
    int c = 3;
    double d = a / (b - c);
    std::cout << d << "\n";
}