#include "movepicker.h"

static const int PIECE_VALUES_MP[7] = {0, 100, 320, 330, 500, 900, 20000};
static constexpr int MP_SEE_THRESHOLD = -82;

MovePicker::MovePicker(Board& b, Move tt, Move k1, Move k2, int p, bool qsearch)
    : board(b), ttMove(tt), ply(p), stage(STAGE_TT), moveCount(0), badCaptureCount(0), currentMoveIndex(0), isQSearch(qsearch) {
    killers[0] = k1;
    killers[1] = k2;
    generate_legacy_moves();
    order_moves();
}

int MovePicker::score_move(Move move) const {
    int score = 0;
    int from = move_from(move);
    int to = move_to(move);
    int piece = piece_at_sq(board, from);
    int flags = move_flags(move);

    if (is_capture(move)) {
        int victimPiece = flags == FLAG_EN_PASSANT ? PAWN : piece_type(board.mailbox[to]);
        int victimValue = PIECE_VALUES_MP[victimPiece];

        int attackerPiece = piece_at_sq(board, from);
        int attackerValue = PIECE_VALUES_MP[piece_type(attackerPiece)];
        int mvvScore = victimValue * 10 - attackerValue;

        if (staticExchangeEvaluation(board, move, MP_SEE_THRESHOLD)) {
            mvvScore += SCORE_GOOD_CAPTURE;
        } else {
            mvvScore += SCORE_BAD_CAPTURE;
        }

        score += mvvScore;
    }

    if (ttMove != 0 && move == ttMove) {
        score += SCORE_TT_MOVE;
    }

    if (is_quiet(move)) {
        if (ply < MAX_PLY && move == killers[0]) {
            score += SCORE_KILLER_1;
        } else if (ply < MAX_PLY && move == killers[1]) {
            score += SCORE_KILLER_2;
        } else {
            score += get_history_score(board.stm, from, to);
            score += get_conhist_score(piece - 1, to, ply);
        }
    }

    return score;
}

void MovePicker::generate_legacy_moves() {
    moveCount = 0;
    currentMoveIndex = 0;
    badCaptureCount = 0;

    if (isQSearch) {
        get_capture_moves(board, moves, moveCount);
    } else {
        get_all_moves(board, moves, moveCount);
    }
}

void MovePicker::order_moves() {
    for (int i = 0; i < moveCount; i++) {
        scores[i] = score_move(moves[i]);
    }

    for (int i = 1; i < moveCount; i++) {
        int tmpScore = scores[i];
        Move tmpMove = moves[i];
        int j = i - 1;
        while (j >= 0 && scores[j] < tmpScore) {
            scores[j + 1] = scores[j];
            moves[j + 1] = moves[j];
            j--;
        }
        scores[j + 1] = tmpScore;
        moves[j + 1] = tmpMove;
    }
}

void MovePicker::score_captures() {
    for (int i = 0; i < moveCount; i++) {
        Move move = moves[i];
        int from = move_from(move);
        int to = move_to(move);
        int flags = move_flags(move);

        int victimPiece = flags == FLAG_EN_PASSANT ? PAWN : piece_type(board.mailbox[to]);
        int victimValue = PIECE_VALUES_MP[victimPiece];

        int attackerPiece = piece_at_sq(board, from);
        int attackerValue = PIECE_VALUES_MP[piece_type(attackerPiece)];

        scores[i] = victimValue * 10 - attackerValue;
    }
}

void MovePicker::score_quiets() {
    for (int i = 0; i < moveCount; i++) {
        Move move = moves[i];
        int from = move_from(move);
        int to = move_to(move);
        int piece = piece_at_sq(board, from);

        scores[i] = get_history_score(board.stm, from, to) + get_conhist_score(piece - 1, to, ply);
    }
}

Move MovePicker::next_scored_move() {
    if (currentMoveIndex >= moveCount) return 0;
    
    int bestScore = -9999999;
    int bestIndex = currentMoveIndex;
    
    for (int i = currentMoveIndex; i < moveCount; i++) {
        if (scores[i] > bestScore) {
            bestScore = scores[i];
            bestIndex = i;
        }
    }
    
    // Swap
    Move bestMove = moves[bestIndex];
    int score = scores[bestIndex];
    
    moves[bestIndex] = moves[currentMoveIndex];
    scores[bestIndex] = scores[currentMoveIndex];
    
    moves[currentMoveIndex] = bestMove;
    scores[currentMoveIndex] = score;
    
    currentMoveIndex++;
    return bestMove;
}

bool MovePicker::has_moves() const {
    return moveCount > 0;
}

Move MovePicker::next_move() {
    while (currentMoveIndex < moveCount) {
        Move move = moves[currentMoveIndex++];
        if (isQSearch && !staticExchangeEvaluation(board, move, 0)) {
            continue;
        }
        return move;
    }
    return 0;
}
