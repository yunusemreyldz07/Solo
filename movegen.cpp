#include "board.h"
#include "bitboard.h"
#include "types.h"
#include <iostream>

enum {PAWN_ALL, PAWN_CAPTURES, PAWN_QUIETS };

namespace {

inline int get_promo_flag(int promo_piece, bool is_capture) {
    int flag = 8;
    if (promo_piece == BISHOP) flag = FLAG_PROMO_BISHOP;
    else if (promo_piece == ROOK) flag = FLAG_PROMO_ROOK;
    else if (promo_piece == QUEEN) flag = FLAG_PROMO_QUEEN;
    if (is_capture) flag += 4;
    return flag;
}

inline void push_move(Move* moves, int fromSq, int toSq, int flag = 0) {
    *moves = static_cast<Move>((flag << 12) | (toSq << 6) | fromSq);
}

inline Bitboard board_occupancy(const Board& board) {
    return board.color[WHITE] | board.color[BLACK];
}

inline bool is_square_attacked_bb(const Board& board, int sq, bool byWhite) {
    const int us = byWhite ? WHITE : BLACK;
    Bitboard occ = board_occupancy(board);

    Bitboard pawns   = board.piece[PAWN   - 1] & board.color[us];
    Bitboard knights = board.piece[KNIGHT - 1] & board.color[us];
    Bitboard bishops = board.piece[BISHOP - 1] & board.color[us];
    Bitboard rooks   = board.piece[ROOK   - 1] & board.color[us];
    Bitboard queens  = board.piece[QUEEN  - 1] & board.color[us];
    Bitboard kings   = board.piece[KING   - 1] & board.color[us];

    if (byWhite) {
        if (pawn_attacks[BLACK][sq] & pawns) return true;
    } else {
        if (pawn_attacks[WHITE][sq] & pawns) return true;
    }

    if (knight_attacks[sq] & knights) return true;
    if (king_attacks[sq] & kings) return true;
    if (get_bishop_attacks(sq, occ) & (bishops | queens)) return true;
    if (get_rook_attacks(sq, occ) & (rooks | queens)) return true;

    return false;
}

} // namespace

void generate_pawn_moves_bb(const Board& board, Move* moves, int& moveCount, int mode) {
    const bool whiteToMove = board.stm == WHITE;
    const int us = whiteToMove ? WHITE : BLACK;

    Bitboard own   = board.color[us];
    Bitboard opp   = board.color[whiteToMove ? BLACK : WHITE];
    Bitboard pawns = board.piece[PAWN - 1] & own;
    Bitboard empty = ~(own | opp);

    while (pawns) {
        int from = lsb(pawns);
        pawns &= pawns - 1;

        if (mode != PAWN_CAPTURES) {
            int to = whiteToMove ? (from + 8) : (from - 8);
            if (to >= 0 && to < 64 && (empty & (1ULL << to))) {
                bool isPromo = whiteToMove ? (from >= 48) : (from <= 15);
                if (isPromo) {
                    for (int promo : {QUEEN, ROOK, BISHOP, KNIGHT}) {
                        push_move(moves + moveCount, from, to, get_promo_flag(promo, false));
                        moveCount++;
                    }
                } else {
                    push_move(moves + moveCount, from, to, FLAG_QUIET);
                    moveCount++;

                    bool onStartRank = whiteToMove ? (from >= 8 && from <= 15) : (from >= 48 && from <= 55);
                    if (onStartRank) {
                        int to2 = whiteToMove ? (from + 16) : (from - 16);
                        if (empty & (1ULL << to2)) {
                            push_move(moves + moveCount, from, to2, FLAG_DOUBLE_PAWN);
                            moveCount++;
                        }
                    }
                }
            }
        }

        if (mode != PAWN_QUIETS) {
            Bitboard attacks = pawn_attacks[us][from] & opp;
            while (attacks) {
                int capSq = lsb(attacks);
                attacks &= attacks - 1;

                bool isPromo = whiteToMove ? (capSq >= 56) : (capSq <= 7);
                if (isPromo) {
                    for (int promo : {QUEEN, ROOK, BISHOP, KNIGHT}) {
                        push_move(moves + moveCount, from, capSq, get_promo_flag(promo, true));
                        moveCount++;
                    }
                } else {
                    push_move(moves + moveCount, from, capSq, FLAG_CAPTURE);
                    moveCount++;
                }
            }

            if (board.enPassant != -1) {
                int epRow = whiteToMove ? 2 : 5;
                int epSq = row_col_to_sq(epRow, board.enPassant);
                if (pawn_attacks[us][from] & (1ULL << epSq)) {
                    push_move(moves + moveCount, from, epSq, FLAG_EN_PASSANT);
                    moveCount++;
                }
            }
        }
    }
}

void generate_knight_moves_bb(const Board& board, Move* moves, int& moveCount, Bitboard targetMask) {
    const bool whiteToMove = board.stm == WHITE;
    const int us = whiteToMove ? WHITE : BLACK;

    Bitboard own     = board.color[us];
    Bitboard opp     = board.color[whiteToMove ? BLACK : WHITE];
    Bitboard knights = board.piece[KNIGHT - 1] & own;

    while (knights) {
        int from = lsb(knights);
        knights &= knights - 1;

        Bitboard targets = knight_attacks[from] & targetMask;
        while (targets) {
            int to = lsb(targets);
            targets &= targets - 1;

            bool isCapture = (opp & (1ULL << to)) != 0;
            push_move(moves + moveCount, from, to, isCapture ? FLAG_CAPTURE : FLAG_QUIET);
            moveCount++;
        }
    }
}

void generate_bishop_moves_bb(const Board& board, Move* moves, int& moveCount, Bitboard targetMask) {
    const bool whiteToMove = board.stm == WHITE;
    const int us = whiteToMove ? WHITE : BLACK;

    Bitboard own     = board.color[us];
    Bitboard opp     = board.color[whiteToMove ? BLACK : WHITE];
    Bitboard bishops = board.piece[BISHOP - 1] & own;
    Bitboard occ     = board_occupancy(board);

    while (bishops) {
        int from = lsb(bishops);
        bishops &= bishops - 1;

        Bitboard targets = get_bishop_attacks(from, occ) & targetMask;
        while (targets) {
            int to = lsb(targets);
            targets &= targets - 1;

            bool isCapture = (opp & (1ULL << to)) != 0;
            push_move(moves + moveCount, from, to, isCapture ? FLAG_CAPTURE : FLAG_QUIET);
            moveCount++;
        }
    }
}

void generate_rook_moves_bb(const Board& board, Move* moves, int& moveCount, Bitboard targetMask) {
    const bool whiteToMove = board.stm == WHITE;
    const int us = whiteToMove ? WHITE : BLACK;

    Bitboard own   = board.color[us];
    Bitboard opp   = board.color[whiteToMove ? BLACK : WHITE];
    Bitboard rooks = board.piece[ROOK - 1] & own;
    Bitboard occ   = board_occupancy(board);

    while (rooks) {
        int from = lsb(rooks);
        rooks &= rooks - 1;

        Bitboard targets = get_rook_attacks(from, occ) & targetMask;
        while (targets) {
            int to = lsb(targets);
            targets &= targets - 1;

            bool isCapture = (opp & (1ULL << to)) != 0;
            push_move(moves + moveCount, from, to, isCapture ? FLAG_CAPTURE : FLAG_QUIET);
            moveCount++;
        }
    }
}

void generate_queen_moves_bb(const Board& board, Move* moves, int& moveCount, Bitboard targetMask) {
    const bool whiteToMove = board.stm == WHITE;
    const int us = whiteToMove ? WHITE : BLACK;

    Bitboard own    = board.color[us];
    Bitboard opp    = board.color[whiteToMove ? BLACK : WHITE];
    Bitboard queens = board.piece[QUEEN - 1] & own;
    Bitboard occ    = board_occupancy(board);

    while (queens) {
        int from = lsb(queens);
        queens &= queens - 1;

        Bitboard targets = (get_bishop_attacks(from, occ) | get_rook_attacks(from, occ)) & targetMask;
        while (targets) {
            int to = lsb(targets);
            targets &= targets - 1;

            bool isCapture = (opp & (1ULL << to)) != 0;
            push_move(moves + moveCount, from, to, isCapture ? FLAG_CAPTURE : FLAG_QUIET);
            moveCount++;
        }
    }
}

void generate_king_moves_bb(const Board& board, Move* moves, int& moveCount, Bitboard targetMask, bool includeCastling) {
    const bool whiteToMove = board.stm == WHITE;
    const int us = whiteToMove ? WHITE : BLACK;

    Bitboard own   = board.color[us];
    Bitboard opp   = board.color[whiteToMove ? BLACK : WHITE];
    Bitboard kings = board.piece[KING - 1] & own;
    if (!kings) return;

    int from = lsb(kings);

    Bitboard targets = king_attacks[from] & targetMask;
    while (targets) {
        int to = lsb(targets);
        targets &= targets - 1;

        bool isCapture = (opp & (1ULL << to)) != 0;
        push_move(moves + moveCount, from, to, isCapture ? FLAG_CAPTURE : FLAG_QUIET);
        moveCount++;
    }

    if (!includeCastling) return;

    Bitboard occ = board_occupancy(board);
    const bool opponentIsWhite = !whiteToMove;

    if (whiteToMove && from == 4) {
        if (board.castling & CASTLE_WK) {
            const Bitboard emptyMask = (1ULL << 5) | (1ULL << 6);
            const bool rookPresent = (board.piece[ROOK - 1] & board.color[WHITE]) & (1ULL << 7);
            if ((occ & emptyMask) == 0 &&
                !is_square_attacked_bb(board, 4, opponentIsWhite) &&
                !is_square_attacked_bb(board, 5, opponentIsWhite) &&
                !is_square_attacked_bb(board, 6, opponentIsWhite) &&
                rookPresent) {
                push_move(moves + moveCount, 4, 6, FLAG_CASTLE_KING);
                moveCount++;
            }
        }
        if (board.castling & CASTLE_WQ) {
            const Bitboard emptyMask = (1ULL << 1) | (1ULL << 2) | (1ULL << 3);
            const bool rookPresent = (board.piece[ROOK - 1] & board.color[WHITE]) & (1ULL << 0);
            if ((occ & emptyMask) == 0 &&
                !is_square_attacked_bb(board, 4, opponentIsWhite) &&
                !is_square_attacked_bb(board, 3, opponentIsWhite) &&
                !is_square_attacked_bb(board, 2, opponentIsWhite) &&
                rookPresent) {
                push_move(moves + moveCount, 4, 2, FLAG_CASTLE_QUEEN);
                moveCount++;
            }
        }
    }

    if (!whiteToMove && from == 60) {
        if (board.castling & CASTLE_BK) {
            const Bitboard emptyMask = (1ULL << 61) | (1ULL << 62);
            const bool rookPresent = (board.piece[ROOK - 1] & board.color[BLACK]) & (1ULL << 63);
            if ((occ & emptyMask) == 0 &&
                !is_square_attacked_bb(board, 60, opponentIsWhite) &&
                !is_square_attacked_bb(board, 61, opponentIsWhite) &&
                !is_square_attacked_bb(board, 62, opponentIsWhite) &&
                rookPresent) {
                push_move(moves + moveCount, 60, 62, FLAG_CASTLE_KING);
                moveCount++;
            }
        }
        if (board.castling & CASTLE_BQ) {
            const Bitboard emptyMask = (1ULL << 57) | (1ULL << 58) | (1ULL << 59);
            const bool rookPresent = (board.piece[ROOK - 1] & board.color[BLACK]) & (1ULL << 56);
            if ((occ & emptyMask) == 0 &&
                !is_square_attacked_bb(board, 60, opponentIsWhite) &&
                !is_square_attacked_bb(board, 59, opponentIsWhite) &&
                !is_square_attacked_bb(board, 58, opponentIsWhite) &&
                rookPresent) {
                push_move(moves + moveCount, 60, 58, FLAG_CASTLE_QUEEN);
                moveCount++;
            }
        }
    }
}

bool is_square_attacked(const Board& board, int sq, bool isWhiteAttacker) {
    return is_square_attacked_bb(board, sq, isWhiteAttacker);
}

void get_all_moves(Board& board, Move moves[], int& moveCount) {
    Move pseudoMoves[256];
    int pseudoMoveCount = 0;
    Move legalMoves[256];
    int legalMoveCount = 0;

    const bool sideToMove = board.stm == WHITE;

    generate_pseudo_moves(board, pseudoMoves, pseudoMoveCount);

    for (int i = 0; i < pseudoMoveCount; i++) {
        Move m = pseudoMoves[i];
        board.makeMove(m);
        int kingSq = -1;
        king_square(board, sideToMove, kingSq);
        if (kingSq == -1) {
            board.unmakeMove(m);
            continue;
        }
        if (!is_square_attacked(board, kingSq, board.stm == WHITE)) {
            legalMoves[legalMoveCount++] = m;
        }
        board.unmakeMove(m);
    }

    for (int j = 0; j < legalMoveCount; j++) {
        moves[j] = legalMoves[j];
    }
    moveCount = legalMoveCount;
}

void get_capture_moves(Board& board, Move moves[], int& moveCount) {
    Move pseudoMoves[256];
    int pseudoMoveCount = 0;

    generate_pseudo_captures(board, pseudoMoves, pseudoMoveCount);

    const bool sideToMove = board.stm == WHITE;
    moveCount = 0;

    for (int i = 0; i < pseudoMoveCount; i++) {
        Move m = pseudoMoves[i];
        board.makeMove(m);
        int kingSq = -1;
        king_square(board, sideToMove, kingSq);
        if (kingSq == -1) {
            board.unmakeMove(m);
            continue;
        }
        if (!is_square_attacked(board, kingSq, board.stm == WHITE)) {
            moves[moveCount++] = m;
        }
        board.unmakeMove(m);
    }
}

void generate_pseudo_moves(const Board& board, Move* moves, int& moveCount) {
    const Bitboard own        = board.color[board.stm];
    const Bitboard targetMask = ~own;

    generate_pawn_moves_bb(board, moves, moveCount, PAWN_ALL);
    generate_knight_moves_bb(board, moves, moveCount, targetMask);
    generate_bishop_moves_bb(board, moves, moveCount, targetMask);
    generate_rook_moves_bb(board, moves, moveCount, targetMask);
    generate_queen_moves_bb(board, moves, moveCount, targetMask);
    generate_king_moves_bb(board, moves, moveCount, targetMask, true);
}

void generate_pseudo_captures(const Board& board, Move* moves, int& moveCount) {
    const Bitboard opp = board.color[other_color(board.stm)];

    generate_pawn_moves_bb(board, moves, moveCount, PAWN_CAPTURES);
    generate_knight_moves_bb(board, moves, moveCount, opp);
    generate_bishop_moves_bb(board, moves, moveCount, opp);
    generate_rook_moves_bb(board, moves, moveCount, opp);
    generate_queen_moves_bb(board, moves, moveCount, opp);
    generate_king_moves_bb(board, moves, moveCount, opp, false);
}

void generate_pseudo_quiets(const Board& board, Move* moves, int& moveCount) {
    const Bitboard own   = board.color[board.stm];
    const Bitboard empty = ~(own | board.color[other_color(board.stm)]);

    generate_pawn_moves_bb(board, moves, moveCount, PAWN_QUIETS);
    generate_knight_moves_bb(board, moves, moveCount, empty);
    generate_bishop_moves_bb(board, moves, moveCount, empty);
    generate_rook_moves_bb(board, moves, moveCount, empty);
    generate_queen_moves_bb(board, moves, moveCount, empty);
    generate_king_moves_bb(board, moves, moveCount, empty, true);
}

bool is_move_pseudo_legal(const Board& board, Move move) {
    if (move == 0) return false;

    const int from  = move_from(move);
    const int to    = move_to(move);
    const int flags = move_flags(move);

    if (from == to) return false;
    if (flags == 6 || flags == 7) return false; // unused flags

    const int us   = board.stm;
    const int them = other_color(us);

    const int pc = board.mailbox[from];
    if (pc == EMPTY || piece_color(pc) != us) return false;

    const int pt       = piece_type(pc);
    const int captured = board.mailbox[to];

    // Can't take your own piece
    if (captured != EMPTY && piece_color(captured) == us) return false;

    // if the taken piece is a king then it's illegal :')
    if (captured != EMPTY && piece_type(captured) == KING) return false;

    // Castling
    if (flags == FLAG_CASTLE_KING || flags == FLAG_CASTLE_QUEEN) {
        if (pt != KING) return false;

        const Bitboard occ = board_occupancy(board);
        const bool oppIsWhite = (them == WHITE);

        if (flags == FLAG_CASTLE_KING) {
            if (us == WHITE) {
                return from == 4 && to == 6 &&
                       (board.castling & CASTLE_WK) &&
                       board.mailbox[7] == W_ROOK &&
                       (occ & ((1ULL << 5) | (1ULL << 6))) == 0 &&
                       !is_square_attacked_bb(board, 4, oppIsWhite) &&
                       !is_square_attacked_bb(board, 5, oppIsWhite) &&
                       !is_square_attacked_bb(board, 6, oppIsWhite);
            } else {
                return from == 60 && to == 62 &&
                       (board.castling & CASTLE_BK) &&
                       board.mailbox[63] == B_ROOK &&
                       (occ & ((1ULL << 61) | (1ULL << 62))) == 0 &&
                       !is_square_attacked_bb(board, 60, oppIsWhite) &&
                       !is_square_attacked_bb(board, 61, oppIsWhite) &&
                       !is_square_attacked_bb(board, 62, oppIsWhite);
            }
        } else { // FLAG_CASTLE_QUEEN
            if (us == WHITE) {
                return from == 4 && to == 2 &&
                       (board.castling & CASTLE_WQ) &&
                       board.mailbox[0] == W_ROOK &&
                       (occ & ((1ULL << 1) | (1ULL << 2) | (1ULL << 3))) == 0 &&
                       !is_square_attacked_bb(board, 4, oppIsWhite) &&
                       !is_square_attacked_bb(board, 3, oppIsWhite) &&
                       !is_square_attacked_bb(board, 2, oppIsWhite);
            } else {
                return from == 60 && to == 58 &&
                       (board.castling & CASTLE_BQ) &&
                       board.mailbox[56] == B_ROOK &&
                       (occ & ((1ULL << 57) | (1ULL << 58) | (1ULL << 59))) == 0 &&
                       !is_square_attacked_bb(board, 60, oppIsWhite) &&
                       !is_square_attacked_bb(board, 59, oppIsWhite) &&
                       !is_square_attacked_bb(board, 58, oppIsWhite);
            }
        }
    }

    // En passant
    if (flags == FLAG_EN_PASSANT) {
        if (pt != PAWN || board.enPassant == -1 || captured != EMPTY) return false;

        const int epRow = (us == WHITE) ? 2 : 5;
        const int epSq  = row_col_to_sq(epRow, board.enPassant);
        if (to != epSq) return false;

        const int capSq = (us == WHITE) ? (to - 8) : (to + 8);
        if (board.mailbox[capSq] != make_piece(PAWN, them)) return false;

        return (pawn_attacks[us][from] & (1ULL << to)) != 0;
    }

    // Pawn moves
    if (pt == PAWN) {
        const bool capFlag = is_capture(move);
        if (capFlag != (captured != EMPTY)) return false;

        const bool toPromoRank = (us == WHITE) ? (to >= 56) : (to <= 7);
        if (is_promotion(move) != toPromoRank) return false;

        // pawn take (take+promotion aswell)
        if (capFlag) {
            return (pawn_attacks[us][from] & (1ULL << to)) != 0;
        }

        const int push = (us == WHITE) ? 8 : -8;

        // double push
        if (flags == FLAG_DOUBLE_PAWN) {
            const bool onStartRank = (us == WHITE) ? (from >= 8 && from <= 15)
                                                   : (from >= 48 && from <= 55);
            return onStartRank && (to == from + 2 * push) && (board.mailbox[from + push] == EMPTY);
        }

        // one push (push + promo aswell)
        return to == from + push;
    }

    // All others
    if (flags != FLAG_QUIET && flags != FLAG_CAPTURE) return false;
    if ((flags == FLAG_CAPTURE) != (captured != EMPTY)) return false;

    const Bitboard toBit = 1ULL << to;
    const Bitboard occ   = board_occupancy(board);

    switch (pt) {
        case KNIGHT: return (knight_attacks[from] & toBit) != 0;
        case BISHOP: return (get_bishop_attacks(from, occ) & toBit) != 0;
        case ROOK:   return (get_rook_attacks(from, occ) & toBit) != 0;
        case QUEEN:  return ((get_bishop_attacks(from, occ) | get_rook_attacks(from, occ)) & toBit) != 0;
        case KING:   return (king_attacks[from] & toBit) != 0;
        default:     return false;
    }
}
