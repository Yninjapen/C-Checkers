#include "cpu.hpp"

cpu::cpu(int cpu_color, int cpu_depth){
    color = cpu_color;
    opponent = 1 - color;
    max_depth = cpu_depth;
    eval_multiplier = opponent * 2 - 1;
}

//returns the nubmer of 1 bits in a bitboard
//aka the "Hamming Weight"
int cpu::count_bits(long long bb){
    int count = 0;
    while(bb){
        long long ls1b = bb & -bb;
        count++;
        bb&= bb-1;
    }
    return count;
}

//returns the cpu's evaluation of the position
double cpu::evaluate(Board board){
    int result = board.game_over;
    if (result){
        if (result == color + 1){
            return 1000;
        }
        if (result == opponent + 1){
            return -1000;
        }
        return 0;
    }
    const double red_pieces = count_bits(board.red_bb);
    const double black_pieces = count_bits(board.black_bb);
    const double total_pieces = red_pieces + black_pieces;
    const double red_kings = count_bits(board.red_bb & board.king_bb);
    const double black_kings = count_bits(board.black_bb & board.king_bb);

    return (((red_pieces - black_pieces)/total_pieces) * 12 + ((red_kings - black_kings)/total_pieces) * 3) * eval_multiplier;
}

double cpu::minimax(Board board, int depth, double alpha, double beta, bool isMaximizingPlayer){
    double score = evaluate(board);

    if (score == 1000){
        return score - (max_depth - depth);
    }
    if (score == -1000){
        return score + (max_depth - depth);
    }
    if (board.game_over){
        return 0;
    }
    if (depth == 0){
        return score;
    }

    if (isMaximizingPlayer){
        double bestVal = -INFINITY;
        std::vector<Move> legal_moves = board.legal_moves;
        for (int i = 0; i < legal_moves.size(); i++){
            board.push_move(legal_moves[i]);
            bestVal = std::max(bestVal, minimax(board, depth - 1, alpha, beta, !isMaximizingPlayer));
            alpha = std::max(alpha, bestVal);
            board.undo();

            if (beta <= alpha){
                break;
            }
        }
        return bestVal;
    }
    else{
        double bestVal = INFINITY;
        std::vector<Move> legal_moves = board.legal_moves;
        for (int i = 0; i < legal_moves.size(); i++){
            board.push_move(legal_moves[i]);
            bestVal = std::min(bestVal, minimax(board, depth - 1, alpha, beta, !isMaximizingPlayer));
            beta = std::min(beta, bestVal);
            board.undo();

            if (beta <= alpha){
                break;
            }
        }
        return bestVal;
    }
}

Move cpu::find_best_move(Board board){
    std::cout << "calculating... \n";
    double bestVal = -INFINITY;
    double alpha = -INFINITY;
    double beta = INFINITY;
    double moveVal;
    Move bestMove;
    std::vector<Move> legal_moves = board.legal_moves;

    for (int i = 0; i < legal_moves.size(); i++){
        board.push_move(legal_moves[i]);
        moveVal = minimax(board, max_depth, alpha, beta, false);

        if (moveVal > bestVal){
            bestVal = moveVal;
            bestMove = legal_moves[i];
        }

        alpha = std::max(alpha, bestVal);
        board.undo();
    }

    std::cout << "The best move has a value of " << bestVal;
    return bestMove;
}