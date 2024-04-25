#include "board.hpp"
#include "misc.hpp"

int main(){
    Board board;
    Move moves[64];
    board.gen_moves(moves);

    double limit = 60000;
    double start = get_time();
    int count = 0;
    while(get_time() - start < limit){
        board.gen_moves(moves);
        count++;
    }

    std::cout << "Time elapsed: " << get_time() - start << " ms\n";
    std::cout << count/limit << " per second\n";
    std::cout << "Total: " << count;
}