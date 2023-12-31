#include "misc.hpp"

//returns the nubmer of 1 bits in a bitboard
//aka the "Hamming Weight"
int count_bits(long long bb){
    int count = 0;
    while(bb){
        long long ls1b = bb & -bb;
        count++;
        bb&= bb-1;
    }
    return count;
}

//converts a square index (according to the chart above) to its bitboard representation
long long square_to_binary(const int square){
   long long n = 0b1;
   n = n << (square - 1);
   return n;
}

//gets the square index of a binary number
//NOTE: THERE CAN ONLY BE ONE 1 BIT
int binary_to_square(const long long binary){
   return log2(binary) + 1;
}

//gets the squares of all 1 bits in a bitboard
std::vector<int> serialize_bb(long long bb){
   std::vector<int> squares;
   if(bb == 0){return squares;}

   while (bb){
      long long ls1b = bb & -bb; // isolate LS1B
      squares.push_back(binary_to_square(ls1b)); //add squares to vector
      bb &= bb-1; // reset LS1B
   }
   return squares;
}

//prints a binary representation of a number
void print_binary(long long num){
   std::cout << std::bitset<32>(num) << "\n";
}

//hashes the bitboard
unsigned int hash_bb(unsigned long long reds, unsigned long long blacks, unsigned long long kings, int turn){
   const unsigned int hash = ((reds * 37 + blacks) * 37 + kings) * 37 + turn;
   return hash;
}

//returns time in milliseconds
double get_time(){
   return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}