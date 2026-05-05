#ifndef MOVEPICKER_H
#define MOVEPICKER_H

#include "board.h"
#include "history.h"
#include "evaluation.h" // for staticExchangeEvaluation

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

    int score_move(Move move) const;
    void score_captures();
    void score_quiets();
    Move next_scored_move();
    bool is_legal(Move move);

public:
    MovePicker(Board& b, Move tt, Move k1, Move k2, int p, bool qsearch = false);

    bool has_moves() const;
    Move next_move();
};

#endif
