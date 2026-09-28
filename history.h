#ifndef HISTORY_H
#define HISTORY_H

#include "board.h"

struct MoveInfo {
    int piece; // 0-11 (colored piece index), -1 for invalid/null
    int to;    // 0-63, -1 for invalid
};
constexpr int CORRHIST_SIZE = 1 << 14; // 16384
constexpr int CORRHIST_SCALE = 64;
// History table: [color][fromSquare][toSquare]
extern int historyTable[2][64][64];
extern int conhistTable[12][64][12][64]; // [prevPiece][prevTo][currPiece][currTo]
extern thread_local MoveInfo moveStack[MAX_PLY];
extern int16_t pawnCorrectionHistory[2][CORRHIST_SIZE];
extern int16_t nonPawnMaterialCorrectionHistory[2][CORRHIST_SIZE];
void clear_history();
void reset_movestack();
void update_history(const Board& board, int color, int fromSq, int toSq, int depth, const Move badQuiets[256], const int& badQuietCount, int ply);
int get_history_score(int color, int fromSq, int toSq);
int get_conhist_score(int piece, int to, int ply);
int pawn_correction_index(const Board& board);
int nonpawn_material_correction_index(const Board& board);
int16_t corrected_static_eval(const Board& board, int16_t rawEval);
void update_correction_history(const Board& board, int16_t rawEval, int16_t searchEval, int depth);

#endif
