#ifndef MOVEPICKER_H
#define MOVEPICKER_H

#include "board.h"
#include "history.h"
#include "evaluation.h"

// Move ordering scores
inline constexpr int SCORE_TT_MOVE      = 1000000;
inline constexpr int SCORE_GOOD_CAPTURE = 1000000;
inline constexpr int SCORE_KILLER_1     = 900000;
inline constexpr int SCORE_KILLER_2     = 800000;
inline constexpr int SCORE_BAD_CAPTURE  = -100000;
inline constexpr int SCORE_PROMO_QUEEN  = 90000;
inline constexpr int SCORE_PROMO_ROOK   = 80000;
inline constexpr int SCORE_PROMO_BISHOP = -70000;
inline constexpr int SCORE_PROMO_KNIGHT = -60000;

// Piece values used for MVV-LVA scoring
inline constexpr int PIECE_VALUES_MP[7] = {0, 100, 320, 330, 500, 900, 20000};

enum Stage {
    STAGE_TT,
    STAGE_GEN_NOISY,
    STAGE_GOOD_NOISY,
    STAGE_GEN_QUIET,
    STAGE_QUIET,
    STAGE_BAD_NOISY,
    STAGE_DONE
};

class MovePicker {
private:
    Board& board;
    Move ttMove;
    Move killers[2];
    int ply;

    Stage stage;

    Move moves[256];
    int scores[256];
    Move badCaptures[256];
    int moveCount;
    int badCaptureCount;
    int currentMoveIndex;

    bool isQSearch;

    void score_captures();
    void score_quiets();
    Move next_scored_move();
    bool is_legal(Move move);

public:
    MovePicker(Board& b, Move tt, Move k1, Move k2, int p, bool qsearch = false);

    bool has_moves();
    Move next_move();
};

#endif
