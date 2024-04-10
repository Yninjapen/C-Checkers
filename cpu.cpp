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

const int MAX_VAL = 10000;

cpu::cpu(int cpu_color, int cpu_depth){
    color = cpu_color;
    opponent = 1 - color;
    max_depth = cpu_depth;
    current_depth = max_depth;
    eval_multiplier = opponent * 2 - 1;
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
int cpu::eval(Board board){
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
            red_king_count++;
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
            black_king_count++;
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

    int result = red_score - black_score;
    if (board.turn) return -result;
    return result;
}

int cpu::search(Board &board, int depth, int ply, int alpha, int beta){
    nodes_traversed++;
    if (board.game_over){
        if (board.game_over > 0){
            return -MAX_VAL + ply;
        }
        return 0;
    }

    if (search_cancelled){
        return 0;
    }
    int val = -MAX_VAL;
    int mate_value = MAX_VAL - ply;

    // If depth == 0, do a quiescence search
    if (depth < 1) return quiesce(board, alpha, beta);

    int new_depth = depth - 1;
    for (int i = 0; i < board.legal_moves.size(); i++){
        Board board2(board);
        board2.push_move(board.legal_moves[i]);

        val = -search(board2, new_depth, ply + 1, -beta, -alpha);

        if (search_cancelled) return 0;
        if (val > alpha){
            if (val >= beta){
                alpha = beta;
                break;
            }
            //raised_alpha = 1;
            alpha = val;
        }
    }

    return alpha;
}

int cpu::quiesce(Board &board, int alpha, int beta){
    nodes_traversed++;
    if (board.game_over){
        if (board.game_over > 0){
            return -MAX_VAL;
        }
        return 0;
    }
    if (search_cancelled){
        return 0;
    }

    int val = eval(board);
    int stand_pat = val;

    if (val >= beta) return beta;
    if (alpha < val) alpha = val;

    for (int i = 0; i < board.legal_moves.size(); i++){
        if (!(board.legal_moves[i].is_take || board.legal_moves[i].is_promo)) continue;

        Board board2(board);
        board2.push_move(board.legal_moves[i]);
        val = -quiesce(board2, -beta, -alpha);

        if (search_cancelled) return 0;

        if (val > alpha){
            if (val >= beta) return beta;
            alpha = val;
        }
    }
    return alpha;
}

int cpu::search_root(Board &board, int depth, int alpha, int beta){
    int val = 0;
    int best = -MAX_VAL;

    for (int i = 0; i < board.legal_moves.size(); i++){
        Board board2(board);
        board2.push_move(board.legal_moves[i]);

        val = -search(board2, depth - 1, 0, -beta, -alpha);

        if (search_cancelled) break;

        if (val > alpha){
            move_to_make = board.legal_moves[i];
            if (val > beta) return beta;

            alpha = val;
        }
    }
    return alpha;
}

int cpu::search_widen(Board &board, int depth, int val){
    int temp = val;
    int alpha = val - 30;
    int beta = val + 30;
    temp = search_root(board, depth, alpha, beta);
    if (temp <= alpha || temp >= beta){
        if (search_cancelled) return val;
        temp = search_root(board, depth, -MAX_VAL, MAX_VAL);
    }
    return temp;
}
int cpu::search_iterate(Board &board){
    int val;
    int move_count = board.legal_moves.size();
    
    val = search_root(board, 1, -MAX_VAL, MAX_VAL);
    current_depth = 2;
    while (!search_cancelled){
        if (move_count == 1 && current_depth == 5){
            search_cancelled = true;
            break;
        }
        val = search_widen(board, current_depth, val);
        current_depth++;
    }

    return val;
}
/*
Perfoms a minimax search up to the max_depth of the cpu
and returns the best move.
*/
Move cpu::max_depth_search(Board &board, bool feedback){
    if (feedback){
        std::cout << "calculating... \n";
    }

    move_to_make = board.legal_moves[0];

    int val = search_root(board, max_depth, -MAX_VAL, MAX_VAL);

    if (feedback){
        std::cout << "The best move has a value of " << val/75;
    }
    return move_to_make;
}

void cpu::manage_time(){
    while(!search_cancelled){
        if (get_time() - search_start > time_limit){
            search_cancelled = true;
            break;
        }
    }
}

//orders the moves based on the previous searches
//meant to be used only with time_search
std::vector<Move> cpu::order_moves(std::map<Move, int> score_map){
    std::vector<Move> result;
    for (auto const& [key, val] : score_map){
        bool inserted = false;
        for (int i = 0; i < result.size(); i++){
            if (val > score_map[result[i]]){
                result.insert(result.begin() + i, key);
                inserted = true;
                break;
            }
        }
        if (!inserted){
            result.push_back(key);
        }
    }

    return result;
}

/*
Finds the best move, but is limited by a time limit t(seconds)
*/
Move cpu::time_search(Board &board, double t_limit, bool feedback){
    move_to_make = board.legal_moves[0];
    nodes_traversed = 0;
    if (feedback){
        std::cout << "calculating... \n";
    }

    // Starts the time manager
    search_cancelled = false;
    time_limit = t_limit*1000;//converts seconds to milliseconds
    search_start = get_time();
    time_manager manager(*this);
    std::thread t1(manager);

    int val = search_iterate(board);
    
    if (feedback){
        std::cout << "The best move has a value of " << val << ", max depth reached was " << current_depth;
        std::cout << ", time elapsed: " << get_time() - search_start << " milliseconds\n";
        std::cout << "Nodes Traversed: " << nodes_traversed << "\n";
    }

    t1.join();
    return move_to_make;
}   