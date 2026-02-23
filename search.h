#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"

extern void initLMRtables();
void resetNodeCounter();
long long getNodeCounter();

int16_t negamax(Board& board, int depth, int16_t alpha, int16_t beta, int ply);

Move getBestMove(Board& board, int maxDepth, int movetimeMs = -1, const std::vector<uint64_t>& positionHistory = {}, int ply = 0);


#endif
