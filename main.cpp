#include "cpu.hpp"
#include "board.hpp"
#include "transposition.hpp"

#include <iostream>

int main(){
    set_hash_function();
    Board board;
    std::vector<Move> move_history;
    std::vector<uint32_t> king_history;

    int x;
    int player_color; //0 == red, 1 == black
    int cpu_depth = 10;
    double t = 1;
    bool undone = false;
    bool is_depth_search = true;
    bool is_cpu_game = false;

    std::cout << "Play as Black(0) or White(1) (or 2 to have the cpu play itself): ";
    std::cin >> player_color;
    std::cout << "\nTime Search(0) or Depth Search(1)?: ";
    std::cin >> is_depth_search;

    if (is_depth_search){
        std::cout << "\ncpu max depth: ";
        std::cin >> cpu_depth;
    }
    else{
        std::cout << "\ntime limit for cpu (seconds): ";
        std::cin >> t;
    }
    std::cout << "\n";

    if (player_color > 1){
        is_cpu_game = true;
        player_color = 1;
    }
    cpu cpu1(1 - player_color, cpu_depth);
    cpu cpu2(player_color, cpu_depth);

    Move movelist[MAX_MOVES];
    Move m;
    int movecount = board.gen_moves(movelist, (char)-1);

    while(!board.check_win() && !board.check_repetition()){
        board.print();
        if ((board.bb.stm == player_color) && !is_cpu_game){
            for (int i = 0; i < movecount; i++){
                std::cout << i << ": ";
                movelist[i].print_move_info();
                std::cout << ", ";
            }
            std::cout << "\n";
            std::cin >> x;
            if ((0 <= x) && (x < movecount)){
                m = movelist[x];
            }
            else{
                std::cout << move_history.size() << " moves recorded\n";
                if (move_history.size() >= 1){
                    board.undo(move_history[move_history.size() - 1], king_history[king_history.size() - 2]);
                    move_history.pop_back();
                    king_history.pop_back();
                }
                if (move_history.size() >= 1){
                    board.undo(move_history[move_history.size() - 1], king_history[king_history.size() - 2]);
                    move_history.pop_back();
                    king_history.pop_back();
                }
                undone = true;
            }
            // if (is_depth_search){
            //     m = cpu2.max_depth_search(board, true);
            // }
            // else{
            //     m = cpu2.time_search(board, t);
            // }
        }
        else{
            if (is_depth_search){
                m = cpu1.max_depth_search(board, true);
            }
            else{
                m = cpu1.time_search(board, t);
            }
        }

        if (!undone) {
            board.push_move(m);
            move_history.push_back(m);
            king_history.push_back(board.bb.kings);
        }
        undone = false;
        movecount = board.gen_moves(movelist, (char)-1);
    }

    board.print();
}