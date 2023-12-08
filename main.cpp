#include <iostream>
#include "headers.hpp"

int main(){
    Board board;
    int x;
    int player_turn = 0;
    cpu CPU(1, 10);

    while(!board.game_over){
        board.print_board();
        if (board.turn == player_turn){
            for (int i = 0; i < board.legal_moves.size(); i++){
                std::cout << i << ": ";
                board.legal_moves[i].get_move_info(board.get_all_pieces());
                std::cout << ", ";
            }
            std::cout << "\n";
            std::cin >> x;
            if ((0 <= x) && (x < board.legal_moves.size())){
                Move m = board.legal_moves[x];
                board.push_move(m);
            }
        }
        else{
            Move m = CPU.find_best_move(board);
            board.push_move(m);
        }
    }

    board.print_board();
    if (board.game_over == 1){
        std::cout << "Red wins\n";
    }
    else if (board.game_over == 2){
        std::cout << "Black wins\n";
    }
    else{
        std::cout << "Its a draw\n";
    }
}