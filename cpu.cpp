/* IMPROVEMENT IDEAS
https://en.wikipedia.org/wiki/Killer_heuristic
https://www.chessprogramming.org/Aspiration_Windows
https://stackoverflow.com/questions/70050677/alpha-beta-pruning-fail-hard-vs-fail-soft
https://www.chessprogramming.org/Window
https://github.com/maksimKorzh/chess_programming/blob/master/didactic_engines/cpw-engine/search.cpp
https://mediocrechess.blogspot.com/2007/01/guide-aspiration-windows-killer-moves.html
*/

//VERSION 1.0
#include "cpu.hpp"

cpu::cpu(int cpu_color, int cpu_depth){
    color = cpu_color;
    opponent = 1 - color;
    max_depth = cpu_depth;
    current_depth = max_depth;
    eval_multiplier = opponent * 2 - 1;
    init_tables();
}

//Initializes all the tables necessary for evaluation.
//Called only once, when the cpu is initialized.
void cpu::init_tables(){
}

//changes the color that the cpu plays for
void cpu::set_color(int new_color){
    color = new_color;
    opponent = 1 - color;
    eval_multiplier = opponent * 2 - 1;
}

//sets the depth of the cpu
void cpu::set_depth(int new_depth){
    max_depth = new_depth;
}

//returns the cpu's evaluation of the position
double cpu::evaluate(Board board){
    uint32_t temp_red = board.red_bb;
    uint32_t temp_black = board.black_bb;

    uint32_t red_moves = board.get_red_movers();
    uint32_t black_moves = board.get_black_movers();

    double red_score = 0;
    double black_score = 0;
    int red_piece_count = 0;
    int black_piece_count = 0;
    int red_king_count = 0;
    int black_king_count = 0;
    uint32_t piece;
    //gets relevent data for red pieces
    while (temp_red){
        piece = temp_red & -temp_red;
        red_score += 75;
        red_piece_count++;
        if (piece & board.king_bb){
            red_score += 25;
            if (piece & SINGLE_EDGE)
                red_score -= 10;
            else if (piece & CENTER_8){
                red_score += 10;
            }
        }
        if (piece & red_moves){
            red_score += 10;
        }
        temp_red &= temp_red-1;
    }

    //gets relevent data for black pieces
    while (temp_black){
        piece = temp_black & -temp_black;
        black_score += 75;
        black_piece_count++;
        if (piece & board.king_bb){
            black_score += 25;
            if (piece & SINGLE_EDGE){
                black_score -= 10;
            }
            else if (piece & CENTER_8){
                black_score += 10;
            }
        }
        if (piece & black_moves){
            black_score += 10;
        }
        temp_black &= temp_black-1;
    }

    if (red_king_count + black_king_count > (red_piece_count + black_piece_count)*.75)// loosely checks if it is the endgame
    {
        if ((red_piece_count > black_piece_count))
        {
            // In losing endgame situations, encourages king moves towards the double corners
            if (board.black_bb & DOUBLE_CORNER & board.king_bb) black_score += 10;
            if (red_king_count == red_piece_count) red_score += 75;
            if (red_piece_count >= black_piece_count + 2) red_score += (black_piece_count == 1) ? 150 : 75;
        }
        else if ((black_piece_count > red_piece_count) && (board.red_bb & DOUBLE_CORNER & board.king_bb))
        {
            if (board.red_bb & DOUBLE_CORNER & board.king_bb) red_score += 10;
            if (black_king_count == red_piece_count) black_score += 75;
            if (black_piece_count >= red_piece_count + 2) black_score += (red_piece_count == 1) ? 150 : 75;
        }
    }

    return ((red_score - black_score) * eval_multiplier);
}

//performs a recursive minimax search
double cpu::minimax(Board &board, int depth, double alpha, double beta){
    nodes_traversed++;
    if (board.game_over){
        if (board.game_over == color + 1){
            return 10000 - (current_depth - depth);
        }
        else if (board.game_over == opponent + 1){
            return -10000 + (current_depth - depth);
        }
        return 0;
    }

    if (depth == 0 || search_cancelled){
        return evaluate(board);
    }

    if (board.turn == color){
        double bestVal = -INFINITY;
        for (int i = 0; i < board.legal_moves.size(); i++){
            Board newBoard(board);
            newBoard.push_move(board.legal_moves[i]);

            bestVal = std::max(bestVal, minimax(newBoard, depth - 1, alpha, beta));
            alpha = std::max(alpha, bestVal);

            if (beta <= alpha){
                break;
            }
        }
        return bestVal;
    }
    else{
        double bestVal = INFINITY;
        for (int i = 0; i < board.legal_moves.size(); i++){
            Board newBoard(board);
            newBoard.push_move(board.legal_moves[i]);

            bestVal = std::min(bestVal, minimax(newBoard, depth - 1, alpha, beta));
            beta = std::min(beta, bestVal);

            if (beta <= alpha){
                break;
            }
        }
        return bestVal;
    }
}

/*
Perfoms a minimax search up to the max_depth of the cpu
and returns the best move.
*/
Move cpu::max_depth_search(Board &board, bool feedback){
    if (feedback){
        std::cout << "calculating... \n";
    }

    current_depth = max_depth;
    double bestVal = -INFINITY;
    double alpha = -INFINITY;
    double beta = INFINITY;
    double moveVal;
    Move bestMove;
    std::vector<Move> legal_moves = board.legal_moves;

    for (int i = 0; i < legal_moves.size(); i++){
        Board board2(board);
        board2.push_move(legal_moves[i]);
        moveVal = minimax(board2, max_depth, alpha, beta);

        if (moveVal > bestVal){
            bestVal = moveVal;
            bestMove = legal_moves[i];
        }

        alpha = std::max(alpha, bestVal);
    }

    if (feedback){
        std::cout << "The best move has a value of " << bestVal;
    }
    return bestMove;
}

void cpu::manage_time(){
    while(!search_cancelled){
        if (get_time() - search_start > time_limit){
            search_cancelled = true;
            break;
        }
    }
}

/*
Finds the best move, but is limited by a time limit t(seconds)
*/
Move cpu::time_search(Board &board, double t_limit, bool feedback){
    nodes_traversed = 0;
    if (feedback){
        std::cout << "calculating... \n";
    }
    std::vector<Move> legal_moves = board.legal_moves;
    std::unordered_map<int, double> score_map;
    Move bestMove;

    search_cancelled = false;
    time_limit = t_limit*1000;//converts seconds to milliseconds
    search_start = get_time();
    time_manager manager(*this);

    std::thread t1(manager);

    current_depth = 0;
    while(!search_cancelled){
        current_depth++;
        double bestVal = -INFINITY;
        double alpha = -INFINITY;
        double beta = INFINITY;
        double moveVal;

        for (int i = 0; i < legal_moves.size(); i++){
            Board board2(board);
            board2.push_move(legal_moves[i]);
            moveVal = minimax(board2, current_depth, alpha, beta);

            if (!search_cancelled){
                score_map[i] = moveVal;
            }
            else{
                break;
            }

            if (moveVal > bestVal){
                bestVal = moveVal;
            }

            alpha = std::max(alpha, bestVal);

        }
        if (abs(bestVal) > 5000){
            search_cancelled = true;
            break;
        }
    }

    double bestVal = -INFINITY;
    for (int i = 0; i < legal_moves.size(); i++){
        if (score_map[i] > bestVal){
            bestVal = score_map[i];
            bestMove = legal_moves[i];
        }
    }
    if (feedback){
        std::cout << "The best move has a value of " << bestVal << ", max depth reached was " << current_depth;
        std::cout << ", time elapsed: " << get_time() - search_start << " milliseconds\n";
        std::cout << "Nodes Traversed: " << nodes_traversed << "\n";
    }

    t1.join();
    return bestMove;
}   