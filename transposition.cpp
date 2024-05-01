#include "transposition.hpp"

hash_func hash;

uint64_t rand64(){
    static uint64_t next = 1;

    next = next * 1103515245 + 12345;
    return next;
}

void set_hash_function(){
    for (int piece = 0; piece < 2; piece ++){
        for (int color = 0; color < 2; color++){
            for (int sq = 0; sq < 32; sq++){
                hash.HASH_FUNCTION[piece][color][sq] = rand64();
            }
        }
    }
    hash.HASH_COLOR = rand64();
}

int tt_table::set_size(int size){
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

int tt_table::probe(uint64_t boardhash, uint8_t depth, int alpha, int beta, char * best){
    if (!tt_size) return INVALID;

    tt_entry * phashe = &tt[boardhash & tt_size];

    if (phashe->hash == boardhash){
        *best = phashe->bestmove;

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

void tt_table::save(uint64_t boardhash, uint8_t depth, int val, char flags, uint8_t best){
    if (!tt_size) return;
    
    tt_entry * phashe = &tt[boardhash & tt_size];

    if ( (phashe->hash == boardhash) && (phashe->depth > depth) ) return;

    phashe->hash = boardhash;
    phashe->val = val;
    phashe->flags = flags;
    phashe->depth = depth;
    phashe->bestmove = best;
}