#include "transposition.hpp"
#include "cpu.hpp"

hash_func hash;

uint64_t rand64() {
    static uint64_t next = 1;

    next = next * 1103515245 + 12345;
    return next;
}

/*
Sets up the hash function. Note that this MUST be called before
any games can be played, otherwise draws by repetition don't work.
*/
void set_hash_function() {
    for (int piece = 0; piece < 2; piece ++){
        for (int color = 0; color < 2; color++){
            for (int sq = 0; sq < 32; sq++){
                hash.HASH_FUNCTION[piece][color][sq] = rand64();
            }
        }
    }
    hash.HASH_COLOR = rand64();
}

/*
Set up the transposition table. Note that this can only
be called ONE TIME per transposition table otherwise I'm
pretty sure it causes a memory leak.
*/
int tt_table::set_size(int size) {
    free(tt);
    if (size & (size - 1)){
        size--;
        for (int i = 1; i < 32; i=i*2){
            size |= size >> i;
        }
        size++;
        size >>=1;
    }
    if (size < 16){
        tt_size = 0;
        return 0;
    }

    tt_size = (size / sizeof(tt_entry)) - 1;
    tt = (tt_entry *) malloc(size);

    return 0;
}

/*
Checks if a position is already tracked in the table. If the position is there, and its
depth is sufficient, return the value that is saved. Otherwise, return INVALID.
*/
int tt_table::probe(uint64_t boardhash, uint8_t depth, int alpha, int beta, char * best) {
    if (!tt_size) return INVALID;

    /*
    To get the index for this hash, we & together the key and the size of the table.
    This quickly and efficiently gives us the index where this hash SHOULD be, however
    we still have to check whether our key matches with the one we have stored at that index.
    */
    tt_entry * phashe = &tt[boardhash & tt_size];

    /* Checks if the key we have matches the one at the index we calculated */
    if (phashe->hash == boardhash){
        *best = phashe->bestmove;

        /*
        We only trust the value we have stored if that value is from a search 
        that was deeper or as deep as our current search.
        */
        if (phashe->depth >= depth){
            if (phashe->flags == TT_EXACT){
                return phashe->val;
            }
            if ((phashe->flags == TT_ALPHA) && (phashe->val <= alpha)){
                return alpha;
            }
            if ((phashe->flags == TT_BETA) && (phashe->val >= beta)){
                return beta;
            }
        }
    }
    return INVALID;
}

/* Saves an entry into the table. Generally this will overwrite any old data. */
void tt_table::save(uint64_t boardhash, uint8_t depth, int ply, int val, char flags, uint8_t best){
    if (!tt_size) return;

    tt_entry * phashe = &tt[boardhash & tt_size];

    /*
    The only case where we don't overwrite is if we are trying to save 
    a position that has already been searched at a greater depth.
    */
    if ( (phashe->hash == boardhash) && (phashe->depth > depth) ) return;

    /*
    Adjusts the score of winning positions to represent the win distance
    from the current position, instead of from the root.
    */
    if (abs(val) > MAX_VAL - 100){
        if (val > 0) val += ply;
        else         val -= ply;
    }
    phashe->hash = boardhash;
    phashe->val = val;
    phashe->flags = flags;
    phashe->depth = depth;
    phashe->bestmove = best;
}