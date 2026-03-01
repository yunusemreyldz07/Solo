#ifndef HISTORY_H
#define HISTORY_H

#include "board.h"

// History table: [color][fromSquare][toSquare]
// Each square is from 0-63, total 64x64 = 4096 entries
extern int historyTable[2][64][64];
extern Move killerMoves[MAX_PLY][2];

// Killer functions
void add_killer_move(int ply, Move move);
bool is_killer_move(int ply, Move move);
void clear_killer_moves();

// History functions
void clear_history();                          // Reset history table
void update_history(int color, int fromSq, int toSq, int depth, const Move badQuiets[256], const int& badQuietCount); // Update on beta cutoff
int get_history_score(int color, int fromSq, int toSq);  // Get score for move ordering

#endif