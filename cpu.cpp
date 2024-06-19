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

cpu::cpu(int cpu_color, int cpu_depth){
    color = cpu_color;
    opponent = 1 - color;
    max_depth = cpu_depth;
    current_depth = max_depth;
    eval_multiplier = opponent * 2 - 1;
    table.set_size(0x4000000);
    eval_table.set_size(0x4000000);
    std::cout << "TABLE SIZE: " << table.tt_size << "\n";
    std::cout << "EVAL TABLE SIZE: " << eval_table.ett_size << "\n";
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

int cpu::mobility_score(Bitboards board) {
    const uint32_t empty = ~(board.pieces[BLACK] | board.pieces[WHITE]);
    const uint32_t black_kings = board.pieces[BLACK] & board.kings;
    const uint32_t white_kings = board.pieces[WHITE] & board.kings;
    
    uint32_t black_squares = (((board.pieces[BLACK] & MASK_L3) << 3) | ((board.pieces[BLACK] & MASK_L5) << 5)) | (board.pieces[BLACK] << 4);
    if (black_kings)
        black_squares |= (((black_kings & MASK_R3) >> 3) | ((black_kings & MASK_R5) >> 5)) | (black_kings >> 4);
    black_squares &= empty;

    uint32_t white_squares = (((board.pieces[WHITE] & MASK_R3) >> 3) | ((board.pieces[WHITE] & MASK_R5) >> 5)) | (board.pieces[WHITE] >> 4);
    if (white_kings)
        white_squares |= (((white_kings & MASK_L3) << 3) | ((white_kings & MASK_L5) << 5)) | (white_kings << 4);
    white_squares &= empty;

    const uint32_t unique_black_moves = (black_squares & ~white_squares);
    const uint32_t unique_white_moves = (white_squares & ~black_squares);

    uint32_t black_result = ((((unique_black_moves & MASK_R3) >> 3) | ((unique_black_moves & MASK_R5) >> 5)) | (unique_black_moves >> 4)) & board.pieces[BLACK];
    if (black_kings)
        black_result |= ((((unique_black_moves & MASK_L3) << 3) | ((unique_black_moves & MASK_L5) << 5)) | (unique_black_moves << 4)) & black_kings;
    
    uint32_t white_result = ((((unique_white_moves & MASK_L3) << 3) | ((unique_white_moves & MASK_L5) << 5)) | (unique_white_moves << 4)) & board.pieces[WHITE];
    if (white_kings)
        white_result |= ((((unique_white_moves & MASK_R3) >> 3) | ((unique_white_moves & MASK_R5) >> 5)) | (unique_white_moves >> 4)) & white_kings;

    return (count_bits(black_result) - count_bits(white_result)) * 10;
}

int cpu::past_pawns(Bitboards board){
    uint32_t coverage[2] = {northFill(board.pieces[BLACK]), southFill(board.pieces[WHITE])};
    uint32_t king_coverage[2] = {board.pieces[BLACK] & board.kings, board.pieces[WHITE] & board.kings};
    uint32_t paths[2] = {(board.pieces[BLACK] & ~board.kings) & ~coverage[WHITE], (board.pieces[WHITE] & ~board.kings) & ~coverage[BLACK]};
    int black_score = 0;
    int white_score = 0;
    int black_distance = 0;
    int white_distance = 0;
    int turn = board.stm;

    while(paths[BLACK] || paths[WHITE]){
        if (turn) {
            /* Increment Paths */
            paths[WHITE] = (((paths[WHITE] & MASK_R3) >> 3) | ((paths[WHITE] & MASK_R5) >> 5)) | (paths[WHITE] >> 4);
            paths[WHITE] &= ~(coverage[BLACK] | king_coverage[BLACK]);

            /* Increment Coverage */
            coverage[WHITE] |= (((coverage[WHITE] & MASK_R3) >> 3) | ((coverage[WHITE] & MASK_R5) >> 5)) | (coverage[WHITE] >> 4);
            king_coverage[WHITE] |= (((king_coverage[WHITE] & MASK_L3) << 3) | ((king_coverage[WHITE] & MASK_L5) << 5)) | (king_coverage[WHITE] << 4);

            paths[BLACK] &= ~(coverage[WHITE] | king_coverage[WHITE]);

            white_distance++;

            if (paths[WHITE] & PROMO_MASK[WHITE]){
                white_score = 24 - white_distance;
                paths[WHITE] = 0;
            }
            paths[WHITE] &= ~PROMO_MASK[WHITE];
        }
        else{
            /* Increment Paths */
            paths[BLACK] = (((paths[BLACK] & MASK_L3) << 3) | ((paths[BLACK] & MASK_L5) << 5)) | (paths[BLACK] << 4);
            paths[BLACK] &= ~(coverage[WHITE] | king_coverage[WHITE]);

            /* Increment Coverage */
            coverage[BLACK] |= (((coverage[BLACK] & MASK_L3) << 3) | ((coverage[BLACK] & MASK_L5) << 5)) | (coverage[BLACK] << 4);
            king_coverage[BLACK] |= (((king_coverage[BLACK] & MASK_R3) >> 3) | ((king_coverage[BLACK] & MASK_R5) >> 5)) | (king_coverage[BLACK] >> 4);

            paths[WHITE] &= ~(coverage[BLACK] | king_coverage[BLACK]);

            black_distance++;

            if (paths[BLACK] & PROMO_MASK[BLACK]){
                black_score = 24 - black_distance;
                paths[BLACK] = 0;
            }
            paths[BLACK] &= ~PROMO_MASK[BLACK];
        }
        turn = !turn;
    }

    return black_score - white_score;
}

/* returns the cpu's evaluation of the position */
int cpu::eval(Board board){
    int probeval = eval_table.probe(board.hash_key);
    if (probeval != INVALID){
        return probeval;
    }

    uint32_t black_kings = board.bb.pieces[BLACK] & board.bb.kings;
    uint32_t white_kings = board.bb.pieces[WHITE] & board.bb.kings;

    int result = (board.piece_count[BLACK] - board.piece_count[WHITE]) * 75;
    result    += (board.king_count[BLACK] - board.king_count[WHITE]) * 25;
    result    += mobility_score(board.bb);

    uint32_t piece;
    while (black_kings){
        piece = black_kings & -black_kings;
        if (piece & CENTER_8) result += 10;
        else if (piece & SINGLE_EDGE) result -= 10;
        black_kings &= black_kings-1;
    }
    while (white_kings){
        piece = white_kings & -white_kings;
        if (piece & CENTER_8) result -= 10;
        else if (piece & SINGLE_EDGE) result += 10;
        white_kings &= white_kings-1;
    }

    if ((board.piece_count[BLACK] != board.king_count[BLACK]) || (board.piece_count[WHITE] != board.king_count[WHITE])){
        int pawn_score = past_pawns(board.bb);
        result += pawn_score;
    }
    /* dampen based on how close we are to a draw */
    result *= (1 - (float)board.reversible_moves*(0.02));

    /* Adjusts the score to be from the perspective of the player whose turn it is */
    if (board.bb.stm) result = -result;

    eval_table.save(board.hash_key, result);
    return result;
}

/* Called when search() runs into a draw:
    -Rewards drawing when down in material
    -Punishes drawing when up in material
*/
int cpu::draw_eval(Board &board){
    return 0;
}

/*
Goes through a list of moves and checks if any are killer moves from 
previous iterations. Helps with move ordering.
*/
void cpu::set_move_scores(Move * m, int movecount, int ply){
    for (int i = 0; i < movecount; i++){
        m[i].score += history[m[i].color()][m[i].from][m[i].to];
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

    int val = -MAX_VAL;
    int mate_value = MAX_VAL - ply;
    char bestmove;
    char tt_move_index = (char)-1;
    char tt_flag = TT_ALPHA;
    int raised_alpha = 0;
    int reduction_depth = 0;
    int moves_tried = 0;
    int new_depth;

    Move movelist[MAX_MOVES];
    Move current_move;

    _mm_prefetch((char *)&table.tt[board.hash_key & table.tt_size], _MM_HINT_NTA);

    /* Cancels the search if time has run out */
    check_time();
    if (search_cancelled) return 0;

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
    if ((val = table.probe(board.hash_key, depth, alpha, beta, &tt_move_index)) != INVALID){
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

    if (depth < 3
        && !is_pv
        && !board.has_takes
        && movecount > 1
        && abs(beta - 1) > -MAX_VAL + 100) 
    {
        int static_eval = eval(board);
        int eval_margin = 40 * depth;
        if (static_eval - eval_margin >= beta){
            return static_eval - eval_margin;
        }
    }

    /* Loop through all the moves */
    for (int i = 0; i < movecount; i++){
        Board board2(board);
        order_moves(movecount, movelist, i);
        current_move = movelist[i];

        board2.push_move(current_move);

        int start = current_move.from;
        int end = current_move.to;
        int color = current_move.color();

        cutoff[color][start][end] -= 1;
        moves_tried++;
        reduction_depth = 0;
        new_depth = depth - 1;

        /* Late Move Reduction */
        if (!is_pv
        && new_depth > 3
        && moves_tried > 1
        && cutoff[color][start][end] < 50
        && !current_move.captures
        && !current_move.is_promo
        && (start != killers[ply][0].from || end != killers[ply][0].to)
        && (start != killers[ply][1].from || end != killers[ply][1].to)){
            cutoff[color][start][end] = 50;
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
            cutoff[color][start][end] += 6;
            if (val >= beta){

                /*
                If we encounter a good move, we save it as a "killer" move. Then, in future searches,
                we can evaluate these moves first, which massively improves the efficiency of the search.
                */
                if (!current_move.captures && !current_move.is_promo){
                    set_killers(current_move, ply);
                    history[color][start][end] += depth*depth;

                    if (history[current_move.color()][start][end] > KILLER_SORT){
                        for (int cl = 0; cl < 2; cl++)
                            for (int a = 0; a < 32; a++)
                                for (int b = 0; b < 32; b++){
                                    history[cl][a][b] = history[cl][a][b] / 2;
                                }
                    }
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
    if (!search_cancelled) table.save(board.hash_key, depth, ply, alpha, tt_flag, bestmove);

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
    Move movelist[MAX_MOVES];
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

    /* Check if the evaluation causes a beta cutoff */
    if (val >= beta) return beta;

    /* Check if the evaluation becomes the new alpha */
    if (alpha < val) alpha = val;

    for (int i = 0; i < movecount; i++){
        order_moves(movecount, movelist, i);

        /* If the current move is not a take or a promotion, we do not worry about it */
        if (!(movelist[i].captures || movelist[i].is_promo)) continue;

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
    Move movelist[MAX_MOVES];
    int movecount = board.gen_moves(movelist, bestmove);
    int val = 0;
    int best = -MAX_VAL;

    for (int i = 0; i < movecount; i++){
        Board board2(board);

        /* Puts the current best move at the front of the movelist */
        order_moves(movecount, movelist, i);

        board2.push_move(movelist[i]);

        cutoff[movelist[i].color()][movelist[i].from][movelist[i].to] -= 1;

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
                table.save(board.hash_key, depth, -1, beta, TT_BETA, bestmove);
                return beta;
            }

            table.save(board.hash_key, depth, -1, alpha, TT_ALPHA, bestmove);
            alpha = val;
        }
    }

    if (!search_cancelled)
        table.save(board.hash_key, depth, -1, alpha, TT_EXACT, bestmove);

    return alpha;
}

/* Handles narrowing the aspiration window */
int cpu::search_widen(Board &board, int depth, int val){
    int temp = val;
    int searches = 0;
    const int max_searches = 3;

    /* Narrow the search window, using the last search's value as a basis. */
    int window = (depth < 8) ? 50 : (20 + abs(val) / 8);
    int alpha  = val - window;
    int beta   = val + window;

    while(true){

        temp = search_root(board, depth, alpha, beta);
        searches++;

        if ((temp > alpha && temp < beta) || search_cancelled){
            break;
        }

        if      (temp <= alpha) alpha -= window * searches;
        else if (temp >= beta)  beta  += window * searches;

        if (searches >= max_searches){
            alpha = -MAX_VAL;
            beta  =  MAX_VAL;
        }
    }
    if (search_cancelled) return val;

    return temp;
}

int cpu::search_iterate(Board &board){
    int val;
    Move movelist[MAX_MOVES];
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
    if (!m.captures){
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
                history[cl][start][end] = history[cl][start][end] / 8;
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

    Move movelist[MAX_MOVES];
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
    Move movelist[MAX_MOVES];
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