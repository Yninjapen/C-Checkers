#include <iostream>
#include <math.h>
#include <vector>
#include <bitset>
#include "headers.hpp"
#include "move_tables.hpp"

/* Bitboard configuration
    29    30    31    32
 25    26    27    28  
    21    22    23    24
 17    18    19    20
    13    14    15    16
 09    10    11    12  
    05    06    07    08
 01    02    03    04
*/

//prints a binary representation of a number
void print_binary(long long num){
    std::cout << std::bitset<32>(num) << std::endl;
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

//generates moves for any given square
//NOTE: currently configured for black takes
long long gen_mask(const double square){
    int row = ceil(square/4);
    int west = -9;
    int east = -7;
    double target1;
    double target2;

    // if(row%2 == 0){
    //     west = -4;
    //     east = -3;
    // }
    // else{
    //     west = -5;
    //     east = -4;
    // }

    long long bb = 0b0;
    target1 = square + west;
    target2 = square + east;

    if ((1 <= target1 && target1 <= 32) && (abs(row - ceil(target1/4)) == 2)){
        bb = bb | square_to_binary(target1);
    }
    if ((1 <= target2 && target2 <= 32) && (abs(row - ceil(target2/4)) == 2)){
        bb = bb | square_to_binary(target2);
    }
    return bb;
}


int main(){
    long long moves[33];
    for (int i = 1; i <= 32; i++){
        moves[i] = red_takes[i] | black_takes[i];
    }
    for (int i = 1; i <= 32; i++){
        // std::cout << i << ": ";
        // std::vector<int> m = serialize_bb(moves[i]); for (int i = 0; i < m.size(); i++){std::cout << m[i] << ", ";}
        // std::cout << ":" << moves[i] << ": ";
        // print_binary(moves[i]);
        std::cout << moves[i] << ", ";
    }
}