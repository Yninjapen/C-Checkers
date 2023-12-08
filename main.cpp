#include <iostream>
#include "headers.hpp"

int main(){
    Board board;
    int x;
    int player_turn = 0;
    cpu CPU(1, 5);

    while(!board.game_over){
        board.print_board();
        if (board.turn == player_turn){
            std::cin >> x;
            if (x == 1){
                Move m = board.get_random_move();
                board.push_move(m);
            }
            if ((x == 2) && board.move_history.size() > 1){
                board.undo();
            }
        }
        else{
            Move m = CPU.find_best_move(board);
            board.push_move(m);
        }
    }

    board.print_board();
    if (board.game_over = 1){
        std::cout << "Red wins\n";
    }
    else if (board.game_over = 2){
        std::cout << "Black wins\n";
    }
    else{
        std::cout << "Its a draw\n";
    }
}