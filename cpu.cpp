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

int cpu::draw_eval(Board &board){
    int red_count = count_bits(board.red_bb);
    int black_count = count_bits(board.black_bb);
    int result = 0;
    if (red_count > black_count){
        result -= (black_count == 1 || red_count >= black_count + 2) ?  10 : 5; //penalty for drawing when up a piece
    }
    else if (black_count > red_count){
        result += (red_count == 1 || black_count >= red_count + 2) ? 10 : 5;
    }
    if (board.turn) result = -result;
    return result;
}

void cpu::set_move_scores(Move * m, int movecount, int ply){
    for (int i = 0; i < movecount; i++){
        if ((m[i].from == killers[ply][1].from)
        &&  (m[i].to == killers[ply][1].to)
        &&  (m[i].score < KILLER_SORT - 1)){
            m[i].score = KILLER_SORT - 1;
        }

        if ((m[i].from == killers[ply][0].from)
        &&  (m[i].to == killers[ply][0].to)
        &&  (m[i].score < KILLER_SORT)){
            m[i].score = KILLER_SORT;
        }
    }
}

int cpu::search(Board &board, int depth, int ply, int alpha, int beta){
    nodes_traversed++;
    if (search_cancelled){
        return 0;
    }
    int val = -MAX_VAL;
    int mate_value = MAX_VAL - ply;
    int raised_alpha = 0;
    Move movelist[64];
    Move current_move;

    // If depth == 0, do a quiescence search
    if (depth < 1) return quiesce(board, ply + 1, alpha, beta);

    if (board.check_repetition()) return draw_eval(board);

    int movecount = board.gen_moves(movelist);
    set_move_scores(movelist, movecount, ply);
    int new_depth = depth - 1;

    for (int i = 0; i < movecount; i++){
        Board board2(board);
        order_moves(movecount, movelist, i);
        current_move = movelist[i];
        board2.push_move(current_move);

        val = -search(board2, new_depth, ply + 1, -beta, -alpha);

        if (search_cancelled) return 0;
        if (val > alpha){
            if (val >= beta){
                if (!current_move.is_take && !current_move.is_promo){
                    set_killers(current_move, ply);
                }   
                alpha = beta;
                break;
            }
            //raised_alpha = 1;
            alpha = val;
        }
    }

    if (!movecount){// If there are no moves, the side to play loses
        alpha = -MAX_VAL + ply;
    }   
    return alpha;
}

int cpu::quiesce(Board &board, int ply, int alpha, int beta){
    nodes_traversed++;
    if (search_cancelled){
        return 0;
    }

    if (board.check_repetition()){
        return draw_eval(board);
    }

    int val = eval(board);
    int stand_pat = val;

    if (val >= beta) return beta;
    if (alpha < val) alpha = val;

    Move movelist[64];
    int movecount = board.gen_moves(movelist);

    if (!movecount){
        return -MAX_VAL + ply;
    }
    for (int i = 0; i < movecount; i++){
        order_moves(movecount, movelist, i);
        if (!(movelist[i].is_take || movelist[i].is_promo)) continue;

        Board board2(board);
        board2.push_move(movelist[i]);
        val = -quiesce(board2, ply + 1, -beta, -alpha);

        if (search_cancelled) return 0;

        if (val > alpha){
            if (val >= beta) return beta;
            alpha = val;
        }
    }
    return alpha;
}

int cpu::search_root(Board &board, int depth, int alpha, int beta){
    Move movelist[64];
    int movecount = board.gen_moves(movelist);
    int val = 0;

    for (int i = 0; i < movecount; i++){
        Board board2(board);
        order_moves(movecount, movelist, i);
        board2.push_move(movelist[i]);

        val = -search(board2, depth - 1, 0, -beta, -alpha);

        if (search_cancelled) break;

        if (val > alpha){
            move_to_make = movelist[i];
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
    Move movelist[64];
    int move_count = board.gen_moves(movelist);
    
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

void cpu::set_killers(Move m, int ply){
    if (!m.is_take){
        if (m.from != killers[ply][0].from || m.to != killers[ply][0].to){
            killers[ply][1] = killers[ply][0];
        }
        killers[ply][0] = m;
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

    Move movelist[64];
    board.gen_moves(movelist);
    move_to_make = movelist[0];

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
void cpu::order_moves(int movecount, Move * m, int current){
    
    int high = current;

    for(int i=current+1; i < movecount; i++){
        if (m[i].score > m[high].score){
            high = i;
        }
    }

    Move temp = m[high];
    m[high] = m[current];
    m[current] = temp;
}

/*
Finds the best move, but is limited by a time limit t(seconds)
*/
Move cpu::time_search(Board &board, double t_limit, bool feedback){
    Move movelist[64];
    board.gen_moves(movelist);
    move_to_make = movelist[0];
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