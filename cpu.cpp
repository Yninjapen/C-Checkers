/* IMPROVEMENT IDEAS
https://en.wikipedia.org/wiki/Killer_heuristic
https://www.chessprogramming.org/Aspiration_Windows
https://stackoverflow.com/questions/70050677/alpha-beta-pruning-fail-hard-vs-fail-soft
https://www.chessprogramming.org/Window
https://github.com/maksimKorzh/chess_programming/blob/master/didactic_engines/cpw-engine/search.cpp
https://mediocrechess.blogspot.com/2007/01/guide-aspiration-windows-killer-moves.html
*/

//VERSION 3.0
#include "cpu.hpp"
#include "transposition.hpp"

uint8_t bestmove;

cpu::cpu(int cpu_color, int cpu_depth){
    color = cpu_color;
    opponent = 1 - color;
    max_depth = cpu_depth;
    current_depth = max_depth;
    eval_multiplier = opponent * 2 - 1;
    table.set_size(0x4000000);
    std::cout << "TABLE SIZE: " << table.tt_size << "\n";
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

    /* Calculate eval and dampen based on how close we are to a draw. */
    int result = (red_score - black_score) * (1 - (float)board.reversible_moves/(float)board.max_moves_without_take);

    /* Adjusts the score to be from the perspective of the player whose turn it is. */
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
    /* If up in material, deduct from score (deducts double if the opponent has only one piece) */
    if (red_count > black_count){
        result -= (black_count == 1 || red_count >= black_count + 2) ?  10 : 5;
    }
    else if (black_count > red_count){
        result += (red_count == 1 || black_count >= red_count + 2) ? 10 : 5;
    }

    /*Adjusts the score to be from the perspective of the player whose turn it is. */
    if (board.turn) result = -result;
    return result;
}

/*
Goes through a list of moves and checks if any are killer moves from 
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
int cpu::search(Board &board, int depth, int ply, int alpha, int beta, int is_pv){
    nodes_traversed++;

    /* Cancels the search if time has run out */
    check_time();
    if (search_cancelled) return 0;

    int val = -MAX_VAL;
    int mate_value = MAX_VAL - ply;
    char bestmove;
    char tt_move_index = (char)-1;
    char tt_flag = TT_ALPHA;
    int raised_alpha = 0;
    int reduction_depth = 0;
    int moves_tried = 0;
    int new_depth;

    Move movelist[64];
    Move current_move;

    /* Mate distance pruning */
    if (alpha < -mate_value) alpha = -mate_value;
    if (beta > mate_value - 1) beta = mate_value - 1;
    if (alpha >= beta) return alpha;

    /*
    If we are at a leaf node, do a quiescence search. This means
    that we will search until there are no more takes or promotions
    left on the board, to ensure only relatively quiet positions 
    are evaluated.
    */
    if (depth < 1) return quiesce(board, ply, alpha, beta);

    /*
    Checks that the current position is not a draw by repetition
    or 50-move rule.
    */
    if (board.check_repetition()) return draw_eval(board);

    /*
    Checks to see if we've searched this position before. If we have, get
    the saved value and return that instead of doing a whole search.
    */
    if ((val = table.probe(board.hashKey, depth, alpha, beta, &tt_move_index)) != INVALID){
        if (!is_pv || (val > alpha && val < beta)){
            if (abs(val) > MAX_VAL - 100) {
                if (val > 0) val -= ply;
                else         val += ply;
            }
            return val;
        }
    }

    int movecount = board.gen_moves(movelist, tt_move_index);
    set_move_scores(movelist, movecount, ply);
    bestmove = movelist[0].id;

    /* Loop through all the moves */
    for (int i = 0; i < movecount; i++){
        Board board2(board);
        order_moves(movecount, movelist, i);
        current_move = movelist[i];

        /*
        If the move being evaluated is a pawn move or a take, we can erase all
        of the positions that were previously being tracked for repetition, because
        those positions cannot possibly occur again. We check if there are any kings
        on the board first so that we don't unnecessarily try to clear the history when
        there was nothing being tracked in the first place.
        */
        if (board.king_bb
        && (!(board.king_bb & current_move.from) //  Checks if it is a pawn move
        || current_move.pieces_taken)) {              //  Checks if it is a take
            board2.clear_pos_history();
        }

        board2.push_move(current_move);

        int start = binary_to_square(current_move.from);
        int end = binary_to_square(current_move.to);

        cutoff[current_move.color][start][end] -= 1;
        moves_tried++;
        reduction_depth = 0;
        new_depth = depth - 1;

        /* Late Move Reduction */
        if (!is_pv
        && new_depth > 3
        && moves_tried > 3
        && cutoff[current_move.color][start][end] < 50
        && !current_move.is_promo
        && (current_move.from != killers[0][ply].from || current_move.to != killers[0][ply].to)
        && (current_move.from != killers[1][ply].from || current_move.to != killers[1][ply].to)){
            cutoff[current_move.color][start][end] = 50;
            reduction_depth = 1;
            if (moves_tried > 6) reduction_depth += 1;
            new_depth -= reduction_depth;
        }

    re_search:

        /* Principle Variation Search */
        if (!raised_alpha){
            val = -search(board2, new_depth, ply + 1, -beta, -alpha, is_pv);
        }
        else{
            if (-search(board2, new_depth, ply+1, -alpha - 1, -alpha, NO_PV) > alpha){
                val = -search(board2, new_depth, ply+1, -beta, -alpha, IS_PV);
            }
        }

        if (reduction_depth && val > alpha){
            new_depth += reduction_depth;
            reduction_depth = 0;
            goto re_search;
        }

        if (search_cancelled) return 0;

        if (val > alpha){
            bestmove = movelist[i].id;
            cutoff[current_move.color][start][end] += 6;
            if (val >= beta){

                /*
                If we encounter a good move, we save it as a "killer" move. Then, in future searches,
                we can evaluate these moves first, which massively improves the efficiency of the search.
                */
                if (!current_move.pieces_taken && !current_move.is_promo){
                    set_killers(current_move, ply);
                }

                alpha = beta;
                tt_flag = TT_BETA;
                break; /* We have encountered a move so good that there is no point in searching further. */
            }
            tt_flag = TT_EXACT;
            raised_alpha = 1;
            alpha = val;
        }
    }

    /*
    If there are no moves, the game is over, and the side
    whose turn it is to play is the loser.
    */
    if (!movecount){
        bestmove = -1;
        alpha = -MAX_VAL + ply;
    }

    /* If we haven't run out of time, save the position in our transposition table */
    if (!search_cancelled) table.save(board.hashKey, depth, ply, alpha, tt_flag, bestmove);

    return alpha;
}

/* Searches until a quiet position is found. In this case, that means
   that the search will continue until there are no takes or promotions
   available on the board. This usually ensures that long exchanges of
   pieces are calculated all the way through.*/
int cpu::quiesce(Board &board, int ply, int alpha, int beta){
    nodes_traversed++;

    check_time();
    if (search_cancelled) return 0;
    if (board.check_repetition()) return draw_eval(board);

    /* Generate legal moves*/
    Move movelist[64];
    int movecount = board.gen_moves(movelist, (char)-1);

    /* Check if the game is over */
    if (!movecount){
        return -MAX_VAL + ply;
    }

    /* Never end on a position where there is a forced move */
    else if (movecount == 1){
        Board board2(board);
        board2.push_move(movelist[0]);
        return -quiesce(board2, ply + 1, -beta, -alpha);
    }

    /* Get an evaluation of the current position.*/
    int val = eval(board);
    int stand_pat = val;

    /* Check if the evaluation causes a beta cutoff */
    if (val >= beta) return beta;

    /* Check if the evaluation becomes the new alpha */
    if (alpha < val) alpha = val;

    for (int i = 0; i < movecount; i++){
        order_moves(movecount, movelist, i);

        /* If the current move is not a take or a promotion, we do not worry about it */
        if (!(movelist[i].pieces_taken || movelist[i].is_promo)) continue;

        Board board2(board);

        /*
        We know that this move is either a take or a promotion, and
        in either case we should be clearing the pos_history.
        */
        if (board.king_bb) {
            board2.clear_pos_history();
        }

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
    int movecount = board.gen_moves(movelist, bestmove);
    int val = 0;
    int best = -MAX_VAL;

    for (int i = 0; i < movecount; i++){
        Board board2(board);

        /* Puts the current best move at the front of the movelist */
        order_moves(movecount, movelist, i);

        /* Handles clearing the position history */
        if (board.king_bb 
        && (!(board.king_bb & movelist[i].from) //  Checks if it is a pawn move
        || movelist[i].pieces_taken)) {         //  Checks if it is a take
            board2.clear_pos_history();
        }

        board2.push_move(movelist[i]);

        cutoff[movelist[i].color][binary_to_square(movelist[i].from)][binary_to_square(movelist[i].to)] -= 1;

        /*Principle Variation Search*

        This works by assuming that the first move we search will be the best move.
        So, for the first move, we search using the full window of (alpha, beta).
        Note: Move ordering must be very good for this to be effective.
        */
        if (best == -MAX_VAL){
            val = -search(board2, depth - 1, 0, -beta, -alpha, IS_PV);
        }
        else{
            /* If we're not looking at the first move, we search with a reduced window.*/
            if (-search(board2, depth - 1, 0, -alpha - 1, -alpha, NO_PV) > alpha){

                /*
                If for some reason this search yields a value better than what we already have,
                we can no longer assume that the first move was the best one, so we must search again
                with the full window.
                */
                val = -search(board2, depth - 1, 0, -beta, -alpha, IS_PV);
            }
        }

        if (val > best) best = val;

        /*
        If the search was cancelled, move on, because we 
        cannot trust the last search as it was cut short.
        */
        if (search_cancelled) break;

        if (val > alpha){
            /* Update the best move */
            bestmove = movelist[i].id;
            move_to_make = movelist[i];
            if (val > beta){
                table.save(board.hashKey, depth, -1, beta, TT_BETA, bestmove);
                return beta;
            }

            table.save(board.hashKey, depth, -1, alpha, TT_ALPHA, bestmove);
            alpha = val;
        }
    }

    if (!search_cancelled)
        table.save(board.hashKey, depth, -1, alpha, TT_EXACT, bestmove);

    return alpha;
}

/* Handles narrowing the aspiration window */
int cpu::search_widen(Board &board, int depth, int val){
    int temp = val;

    /*
    Narrow the search window, using the last search's value as a basis.
    Using 25 / log10(d + 2) lets us narrow the window as the search gets
    deeper. I haven't seen anyone using this method of narrowing the window 
    the deeper we go, so be aware it may not actually be good.
    */
    int window = 25 / log10(depth + 2);
    int alpha = val - window;
    int beta = val + window;

    temp = search_root(board, depth, alpha, beta);

    /*
    If the narrower search fails, we have to re-search with -MAX_VAL, MAX_VAL as our window. 
    This is very costly, but more often then not narrowing the window saves time.
    */
    if (temp <= alpha || temp >= beta){
        if (search_cancelled) return val;
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
    int move_count = board.gen_moves(movelist, (char)-1);
    
    val = search_root(board, 1, -MAX_VAL, MAX_VAL);
    current_depth = 2;
    /* Searches with increasing depth until the time is up */
    while (!search_cancelled){
        if ((move_count == 1 && current_depth == 5) || abs(val) > 5000){
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
    if (!m.pieces_taken){
        if (m.from != killers[ply][0].from || m.to != killers[ply][0].to){
            killers[ply][1] = killers[ply][0];
        }
        killers[ply][0] = m;
    }
}

void cpu::age_history_table() {
    for (int cl = 0; cl < 2; cl++){
        for (int start = 0; start < 32; start++){
            for (int end = 0; end < 32; end++){
                cutoff[cl][start][end] = 100;
            }
        }
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
    board.gen_moves(movelist, (char)-1);
    move_to_make = movelist[0];
    time_limit = INFINITY;

    int val = search_root(board, max_depth, -MAX_VAL, MAX_VAL);

    if (feedback){
        std::cout << "The best move has a value of " << (double)val/75;
    }
    return move_to_make;
}

/* Orders the moves based on their score. */
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
    board.gen_moves(movelist, (char)-1);
    move_to_make = movelist[0];
    nodes_traversed = 0;
    table.fails = 0;

    if (feedback){
        std::cout << "calculating... \n";
    }

    // Starts the time manager
    search_cancelled = false;
    time_limit = t_limit*1000;//converts seconds to milliseconds
    search_start = get_time();

    int val = search_iterate(board);
    
    if (feedback){
        std::cout << "The best move has a value of " << (double)val/75 << ", max depth reached was " << current_depth - 1;
        std::cout << ", time elapsed: " << get_time() - search_start << " milliseconds\n";
        std::cout << "Nodes Traversed: " << nodes_traversed << "\n";
    }
    return move_to_make;
}