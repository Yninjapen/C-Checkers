#include "board.hpp"
#include "misc.hpp"
#include "transposition.hpp"

#include <cstdint>

const uint64_t VERIFICATION_NUMS[16] = {1, 7, 49, 302, 1469, 7361, 36768, 179740, 845931, 3963680, 18391564, 85242128, 388623673, 1766623630, 7978439499, 36263167175};

uint64_t Perft(Board board, int depth, int ply) {
    Move movelist[MAX_MOVES];
    uint64_t nodes, sumnodes = 0;
    uint32_t prev_kings = 0;

    int movecount = board.gen_moves(movelist, -1);
    if (depth <= 1) return movecount;

    for (int i = 0; i < movecount; i++) {
        prev_kings = board.bb.kings;
        board.push_move(movelist[i]);
        nodes = Perft(board, depth - 1, ply + 1);
        board.undo(movelist[i], prev_kings);
        sumnodes += nodes;
    }
    return sumnodes;
}

int main() {
    std::cout << "Perft depth:";
    int depth;
    std::cin >> depth;
    std::cout << "\n";

    set_hash_function();
    Board board;
    board.reset();

    uint64_t total_nodes = Perft(board, depth, 0);
    int difference = total_nodes - VERIFICATION_NUMS[depth];

    std::cout << "TOTAL NODES UP TO DEPTH " << depth << ": " << total_nodes << "\n";
    std::cout << "EXPECTED NODES AT DEPTH " << depth << ": " << VERIFICATION_NUMS[depth] << "\n";
    std::cout << "DIFFERENCE: " << difference << "\n";
    std::cout << "ERROR: " << ((float)abs(difference) / VERIFICATION_NUMS[depth])*100 << "%\n";
}