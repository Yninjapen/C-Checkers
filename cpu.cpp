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

/* changes the color that the cpu plays for */
void cpu::set_color(int new_color){
    color = new_color;
    opponent = 1 - color;
    eval_multiplier = opponent * 2 - 1;
}

/* sets the depth of the cpu */
void cpu::set_depth(int new_depth){
    max_depth = new_depth;
}

/* returns the cpu's evaluation of the position */
int cpu::eval(Board board){
    uint32_t temp_red = board.red_bb;
    uint32_t temp_black = board.black_bb;

    /* Bitboards of pieces that can move on both sides */
    uint32_t red_moves = board.get_red_movers();
    uint32_t black_moves = board.get_black_movers();

    double red_score = 0;
    double black_score = 0;
    int red_piece_count = 0;
    int black_piece_count = 0;
    int red_king_count = 0;
    int black_king_count = 0;
    uint32_t piece;

    /*Calculates the scores for red and black
        A pawn is worth 75
        A king is worth an extra 25
        -10 for a king on a single edge
        +10 for a king in the center
        +10 for a piece that can move
    */
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

    /*Adjusts the score to be from the perspective of the player whose turn it is*/
    if (board.turn) return -result;
    return result;
}

/* Called when search() runs into a draw:

    -Rewards drawing when down in material
    -Punishes drawing when up in material
*/
int cpu::draw_eval(Board &board){
    int red_count = count_bits(board.red_bb);
    int black_count = count_bits(board.black_bb);
    int result = 0;
    /* If up in material, deduct from score 
       (deducts double if the opponent has only one piece)*/
    if (red_count > black_count){
        result -= (black_count == 1 || red_count >= black_count + 2) ?  10 : 5;
    }
    else if (black_count > red_count){
        result += (red_count == 1 || black_count >= red_count + 2) ? 10 : 5;
    }

    /*Adjusts the score to be from the perspective of the player whose turn it is*/
    if (board.turn) result = -result;
    return result;
}

/* Goes through a list of moves and checks if any are killer moves from 
   previous iterations. Helps with move ordering.
*/
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

/* Recursive Search
    -Uses alpha-beta pruning to reduce the number of nodes explored

Returns an integer evaluation of the position passed in.
*/
int cpu::search(Board &board, int depth, int ply, int alpha, int beta){
    nodes_traversed++;

    /* Cancels the search if time has run out */
    if (search_cancelled) return 0;

    int val = -MAX_VAL;
    int raised_alpha = 0;
    Move movelist[64];
    Move current_move;

    /* If we are at a leaf node, do a quiescence search. This means
       that we will search until there are no more takes or promotions
       left on the board, to ensure only relatively quiet positions 
       are evaluated. */
    if (depth < 1) return quiesce(board, ply + 1, alpha, beta);

    /* Checks that the current position is not a draw by repetition
       or 50-move rule. */
    if (board.check_repetition()) return draw_eval(board);

    int movecount = board.gen_moves(movelist);
    set_move_scores(movelist, movecount, ply);
    int new_depth = depth - 1;

    for (int i = 0; i < movecount; i++){
        Board board2(board);
        order_moves(movecount, movelist, i);
        current_move = movelist[i];

        /* If the move being evaluated is a pawn move or a take, we can erase all
           of the positions that were previously being tracked for repetition, because
           those positions cannot possibly occur again. We check if there are any kings
           on the board first so that we don't unnecessarily try to clear the history when
           there was nothing being tracked in the first place. */
        if (movelist[i].kings && (!(movelist[i].kings & movelist[i].to) || movelist[i].is_take)) {
            board2.clear_pos_history();
        }

        board2.push_move(current_move);

        val = -search(board2, new_depth, ply + 1, -beta, -alpha);

        if (search_cancelled) return 0;
        if (val > alpha){
            if (val >= beta){

                /* If we encounter a good move, we save it as a "killer" move. Then, in future searches,
                   we can evaluate these moves first, which massively improves the efficiency of the search. */
                if (!current_move.is_take && !current_move.is_promo){
                    set_killers(current_move, ply);
                }

                alpha = beta;
                break; /* We have encountered a move so good that there is no point in searching further. */
            }
            alpha = val;
        }
    }

    /* If there are no moves, the game is over, and the side
       whose turn it is to play is the loser. */
    if (!movecount){
        alpha = -MAX_VAL + ply;
    }
    return alpha;
}

/* Searches until a quiet position is found. In this case, that means
   that the search will continue until there are no takes or promotions
   available on the board. This usually ensures that long exchanges of
   pieces are calculated all the way through.*/
int cpu::quiesce(Board &board, int ply, int alpha, int beta){
    nodes_traversed++;
    if (search_cancelled) return 0;
    if (board.check_repetition())return draw_eval(board);

    /* Get an evaluation of the current position.*/
    int val = eval(board);
    int stand_pat = val;

    /* Check if the evaluation causes a beta cutoff */
    if (val >= beta) return beta;

    /* Check if the evaluation becomes the new alpha */
    if (alpha < val) alpha = val;

    Move movelist[64];
    int movecount = board.gen_moves(movelist);

    /* Check if the game is over */
    if (!movecount){
        return -MAX_VAL + ply;
    }

    for (int i = 0; i < movecount; i++){
        order_moves(movecount, movelist, i);

        /* If the current move is not a take or a promotion, we do not worry about it */
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

/* Search the lowest level of the game tree */
int cpu::search_root(Board &board, int depth, int alpha, int beta){
    Move movelist[64];
    int movecount = board.gen_moves(movelist);
    int val = 0;

    for (int i = 0; i < movecount; i++){
        order_moves(movecount, movelist, i);
        Board board2(board);

        /* Handles clearing the position history */
        if (movelist[i].kings && (!(movelist[i].kings & movelist[i].to) || movelist[i].is_take)) {
            board2.clear_pos_history();
        }

        board2.push_move(movelist[i]);

        val = -search(board2, depth - 1, 0, -beta, -alpha);

        /* If the search was cancelled, move on, because we 
           cannot trust the last search as it was cut short. */
        if (search_cancelled) break;

        if (val > alpha){
            /* Update the best move*/
            move_to_make = movelist[i];
            if (val > beta) return beta;

            alpha = val;
        }
    }
    return alpha;
}

/* Handles narrowing the aspiration window */
int cpu::search_widen(Board &board, int depth, int val){
    /* Narrow the search window, using the last search's value as a basis */
    int temp = val;
    int alpha = val - 30;
    int beta = val + 30;

    temp = search_root(board, depth, alpha, beta);

    /* If the narrower search fails, we have to re-search with -INF, INF as our window. 
       This is very costly, but more often then not narrowing the window saves time. */
    if (temp <= alpha || temp >= beta){
        if (search_cancelled){
            return val;
            }
        temp = search_root(board, depth, -MAX_VAL, MAX_VAL);
    }
    if (search_cancelled){
        return val;
    }
    return temp;
}

int cpu::search_iterate(Board &board){
    int val;
    Move movelist[64];
    int move_count = board.gen_moves(movelist);
    
    val = search_root(board, 1, -MAX_VAL, MAX_VAL);
    current_depth = 2;
    /* Searches with increasing depth until the time is up */
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

/* Handles setting the killer moves */
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
        std::cout << "The best move has a value of " << val << ", max depth reached was " << current_depth - 1;
        std::cout << ", time elapsed: " << get_time() - search_start << " milliseconds\n";
        std::cout << "Nodes Traversed: " << nodes_traversed << "\n";
    }

    t1.join();
    return move_to_make;
}   