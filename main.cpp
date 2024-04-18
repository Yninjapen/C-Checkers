#include <iostream>
#include "cpu.hpp"
#include "board.hpp"

int main(){
    Board board;
    std::vector<Move> move_history;
    Move initial_pos;(4095, 4293918720, 0, 1);
    initial_pos.reds = 4095;
    initial_pos.blacks = 4293918720;
    initial_pos.kings = 0;
    initial_pos.color = 1;
    move_history.push_back(initial_pos);

    int x;
    int player_color; //0 == red, 1 == black
    int cpu_depth = 10;
    double t = 1;
    bool undone = false;
    bool is_depth_search = true;
    bool is_cpu_game = false;

    std::cout << "Play as Red(0) or Black(1) (or 2 to have the cpu play itself): ";
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

    Move movelist[64];
    Move m;
    int movecount = board.gen_moves(movelist);

    while(!board.check_win() && !board.check_repetition()){
        board.print_board();

        if ((board.turn == player_color) && !is_cpu_game){
            for (int i = 0; i < movecount; i++){
                std::cout << i << ": ";
                movelist[i].get_move_info(board.get_all_pieces());
                std::cout << ", ";
            }
            std::cout << "\n";
            std::cin >> x;
            if ((0 <= x) && (x < movecount)){
                m = movelist[x];
            }
            else{
                if (move_history.size() >= 2){
                    board.undo(move_history[move_history.size() - 2], move_history[move_history.size() - 1]);
                    move_history.pop_back();
                    undone = true;
                }
                if (move_history.size() >= 2){
                    board.undo(move_history[move_history.size() - 2], move_history[move_history.size() - 1]);
                    move_history.pop_back();
                    undone = true;
                }
            }
        }
        else{
            if (is_depth_search){
                m = cpu1.max_depth_search(board, true);
            }
            else{
                m = cpu1.time_search(board, t);
            }
        }
        
        if (!undone){
            if (m.is_unreversible()){
                board.clear_pos_history();
            }
            board.push_move(m);
            move_history.push_back(m);
        }
        undone = false;
        movelist[64];
        movecount = board.gen_moves(movelist);
    }

    board.print_board();
}