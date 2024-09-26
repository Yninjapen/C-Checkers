#include "board.hpp"
#include "misc.hpp"
#include "transposition.hpp"

#include <cstdint>

// Source: https://www.aartbik.com/MISC/checkers.html
const uint64_t VERIFICATION_NUMS[18] = {1, 7, 49, 302, 1469, 7361, 36768, 179740, 845931, 3963680, 18391564, 85242128, 388623673, 1766623630, 7978439499, 36263167175, 165629569428, 758818810990};
int capture_arr[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

struct Perft_tt_entry {
    uint64_t board_hash;
    uint64_t nodes;
    uint8_t depth;
};

struct Perft_tt_table {
    Perft_tt_entry * tt;
    int tt_size;

    void set_size(int size) {
        free(tt);
        if (size & (size - 1)){
            size--;
            for (int i = 1; i < 32; i=i*2){
                size |= size >> i;
            }
            size++;
            size >>=1;
        }
        if (size < 16){
            tt_size = 0;
        }

        tt_size = (size / sizeof(Perft_tt_entry)) - 1;
        tt = (Perft_tt_entry *) malloc(size);
    }
    uint64_t probe(uint64_t board_hash, uint8_t depth) {
        if (!tt_size) return -INVALID;
        
        Perft_tt_entry * phashe = &tt[board_hash & tt_size];

        if ((phashe->board_hash == board_hash) && (phashe->depth == depth)) return phashe->nodes;

        return -INVALID;
    }
    void save(uint64_t board_hash, uint64_t nodes, uint8_t depth) {
        if (!tt_size) return;

        Perft_tt_entry * phashe = &tt[board_hash & tt_size];

        phashe->board_hash = board_hash;
        phashe->nodes = nodes;
        phashe->depth = depth;
    }
    void close() {
        free(tt);
    }
} table;

uint64_t actual_nodes;

uint64_t Perft(Board board, int depth, int ply) {
    actual_nodes++;
    Move movelist[MAX_MOVES];
    uint64_t nodes, sumnodes = 0;
    uint32_t prev_kings = 0;

    uint64_t probeval = table.probe(board.hash_key, depth);

    if (probeval != -INVALID) {
        return probeval;
    }

    int movecount = board.gen_moves(movelist, -1);
    if (depth <= 1) return (depth > 0)? movecount:1;

    for (int i = 0; i < movecount; i++) {
        Move move = movelist[i];

        prev_kings = board.bb.kings;
        board.push_move(move);

        nodes = Perft(board, depth - 1, ply + 1);
        sumnodes += nodes;

        board.undo(move, prev_kings);
    }

    table.save(board.hash_key, sumnodes, depth);
    return sumnodes;
}

int main() {
    std::cout << "Perft depth: ";
    int depth;
    std::cin >> depth;
    std::cout << "\n";

    set_hash_function();
    table.set_size(0x4000000);
    Board board;
    board.reset();
    actual_nodes = 0;

    uint64_t start = get_time();
    uint64_t total_nodes = Perft(board, depth, 0);
    uint64_t elapsed = get_time() - start;
    int difference = total_nodes - VERIFICATION_NUMS[depth];

    std::cout << "TOTAL NODES UP TO DEPTH " << depth << ": " << total_nodes << "\n";
    std::cout << "EXPECTED NODES AT DEPTH " << depth << ": " << VERIFICATION_NUMS[depth] << "\n";
    std::cout << "DIFFERENCE: " << difference << "\n";
    std::cout << "ERROR: " << ((float)abs(difference) / VERIFICATION_NUMS[depth])*100 << "%\n\n";
    for (int i = 0; i < 9; i++) {
        if (capture_arr[i]) {
            std::cout << "CAPTURES WITH " << i + 1 << " CAPTURES: " << capture_arr[i] << "\n";
        }
    }
    std::cout << "\nTIME ELAPSED: " << (int)elapsed << " ms\n";
    if (elapsed > 0)
        std::cout << "\n" << (int)(actual_nodes/elapsed) << " KNodes/s\n";
    else
        std::cout << "Test completed too fast for accurate speed results.\n";
    table.close();
}