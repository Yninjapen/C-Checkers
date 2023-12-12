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
    //initialize piece_map
    for (int i = 1; i <= 32; i++){
        long long bin = square_to_binary(i); //every square initially gets a value from 1-8 based on
        int row = ceil((double)i/4);         //its row, and a bonus of +1 is given for being on the edge
        red_piece_map[bin] = row; 
        black_piece_map[bin] = 9 - row;      //this is reversed for black, so for black row 8 has a value of 1
        if ((i%4 == 0) || (i%4 == 1)){
            red_piece_map[bin] += 1;         //I don't actually know if the edge bonus is good, just my opinion
            black_piece_map[bin] += 1;
        }
    }
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
    const int result = board.game_over;
    if (result != 0){
        if (result == color + 1){
            return 1000;
        }
        if (result == opponent + 1){
            return -1000;
        }
        return 0;
    }

    unsigned long long temp_red = board.red_bb;
    unsigned long long temp_black = board.black_bb;

    int red_piece_count = 0;
    int red_king_count = 0;
    int black_piece_count = 0;
    int black_king_count = 0;

    double red_pos_val = 0;
    double black_pos_val = 0;

    long long ls1b;
    //gets relevent data for red pieces
    while (temp_red){
        ls1b = temp_red & -temp_red;
        red_piece_count++;
        if (ls1b & board.king_bb){
            red_king_count++;
        }
        else{
            red_pos_val += red_piece_map[ls1b];
        }
        temp_red &= temp_red-1;
    }

    //gets relevent data for black pieces
    while (temp_black){
        ls1b = temp_black & -temp_black;
        black_piece_count++;
        if (ls1b & board.king_bb){
            black_king_count++;
        }
        else{
            black_pos_val += black_piece_map[ls1b];
        }
        temp_black &= temp_black-1;
    }

    double total_pieces = red_piece_count + black_piece_count;
    double piece_val = ((red_piece_count - black_piece_count)/total_pieces) * 12;
    double king_val = ((red_king_count - black_king_count)/total_pieces) * 6;
    double pos_val = (((red_pos_val / (red_piece_count - red_king_count + 1)) - (black_pos_val / (black_piece_count - black_king_count + 1)))/7) * 2;

    return (piece_val + king_val + pos_val) * eval_multiplier;
}

//performs a recursive minimax search
double cpu::minimax(Board board, int depth, double alpha, double beta){
    double score = evaluate(board);

    if (score == 1000){
        return score - (current_depth - depth);
    }
    if (score == -1000){
        return score + (current_depth - depth);
    }
    if (board.game_over){
        return 0;
    }
    if (depth == 0 || search_cancelled){
        return score;
    }

    if (board.turn == color){
        double bestVal = -INFINITY;
        std::vector<Move> legal_moves = board.legal_moves;
        for (int i = 0; i < legal_moves.size(); i++){
            board.push_move(legal_moves[i]);

            bestVal = std::max(bestVal, minimax(board, depth - 1, alpha, beta));
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

            bestVal = std::min(bestVal, minimax(board, depth - 1, alpha, beta));
            beta = std::min(beta, bestVal);
            board.undo();

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
Move cpu::max_depth_search(Board board, bool feedback){
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
        board.push_move(legal_moves[i]);
        moveVal = minimax(board, max_depth, alpha, beta);

        if (moveVal > bestVal){
            bestVal = moveVal;
            bestMove = legal_moves[i];
        }

        alpha = std::max(alpha, bestVal);
        board.undo();
    }

    if (feedback){
        std::cout << "The best move has a value of " << bestVal;
    }
    return bestMove;
}

void cpu::manage_time(){
    while(true){
        if (get_time() - search_start > time_limit){
            search_cancelled = true;
            break;
        }
    }
}

/*
Finds the best move, but is limited by a time limit t(seconds)
*/
Move cpu::time_search(Board board, double t_limit, bool feedback){
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

    current_depth = 1;
    while(!search_cancelled){
        double bestVal = -INFINITY;
        double alpha = -INFINITY;
        double beta = INFINITY;
        double moveVal;

        for (int i = 0; i < legal_moves.size(); i++){
            board.push_move(legal_moves[i]);
            moveVal = minimax(board, current_depth, alpha, beta);

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
            board.undo();

        }
        current_depth++;
    }

    double bestVal = -INFINITY;
    for (int i = 0; i < legal_moves.size(); i++){
        if (score_map[i] > bestVal){
            bestVal = score_map[i];
            bestMove = legal_moves[i];
        }
        std::cout << "move " << i << ": value of " << score_map[i] << "\n";
    }
    if (feedback){
        std::cout << "The best move has a value of " << bestVal << ", max depth reached was " << current_depth << "\n";
    }
    t1.join();
    return bestMove;
}   