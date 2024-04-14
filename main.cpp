#include <iostream>
#include "cpu.hpp"
//#include "new_cpu.hpp"
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

    std::cout << "type 0 to play as Red, 1 to play as Black: ";
    std::cin >> player_color;
    std::cout << "\ncpu max depth: ";
    std::cin >> cpu_depth;
    std::cout << "\ntime limit for cpu (seconds): ";
    std::cin >> t;
    std::cout << "\n";

    cpu cpu1(1 - player_color, cpu_depth);
    cpu cpu2(player_color, cpu_depth);

    double thinking_time = get_time() - 3000;

    Move movelist[64];
    int movecount = board.gen_moves(movelist);

    while(!board.check_win() && !board.check_repetition()){
        board.print_board();

        if (board.turn == player_color){
            thinking_time = get_time();
            for (int i = 0; i < movecount; i++){
                std::cout << i << ": ";
                movelist[i].get_move_info(board.get_all_pieces());
                std::cout << ", ";
            }
            std::cout << "\n";
            std::cin >> x;
            if ((0 <= x) && (x < movecount)){
                Move m = movelist[x];
                board.push_move(m);
                move_history.push_back(m);
            }
            else{
                if (move_history.size() >= 2){
                    board.undo(move_history[move_history.size() - 2], move_history[move_history.size() - 1]);
                    move_history.pop_back();
                }
                if (move_history.size() >= 2){
                    board.undo(move_history[move_history.size() - 2], move_history[move_history.size() - 1]);
                    move_history.pop_back();
                }
            }
            // Move m = cpu2.time_search(board, t);
            // board.push_move(m);
            // move_history.push_back(m);
        }
        else{
            //t = (get_time() - thinking_time)/2000;
            Move m = cpu1.time_search(board, t);
            board.push_move(m);
            move_history.push_back(m);
        }
        movelist[64];
        movecount = board.gen_moves(movelist);
    }

    board.print_board();
}