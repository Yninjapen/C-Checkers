#include <iostream>
#include "cpu.hpp"
#include "board.hpp"

int main(){
    Board board;
    int x;
    int player_color = 1; //0 == red, 1 == black
    int cpu_depth = 11;
    cpu cpu1(1 - player_color, cpu_depth);

    while(!board.game_over){
        board.print_board();
        if (board.turn == player_color){
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
            else if (board.move_history.size() > 1){
                board.undo();
            }
        }
        else{
            Move m = cpu1.time_search(board, 1000);
            board.push_move(m);
        }
    }

    board.print_board();
}