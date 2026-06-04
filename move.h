#ifndef MOVE_H
#define MOVE_H

#include "types.h"
#include "piece.h"

// Move Flags
enum MoveFlags {
    FLAG_QUIET = 0,
    FLAG_DOUBLE_PAWN = 1,
    FLAG_CASTLE_KING = 2,
    FLAG_CASTLE_QUEEN = 3,
    FLAG_CAPTURE = 4,
    FLAG_EN_PASSANT = 5,
    FLAG_PROMO_KNIGHT = 8,
    FLAG_PROMO_BISHOP = 9,
    FLAG_PROMO_ROOK = 10,
    FLAG_PROMO_QUEEN = 11,
    FLAG_PROMO_KNIGHT_CAPTURE = 12,
    FLAG_PROMO_BISHOP_CAPTURE = 13,
    FLAG_PROMO_ROOK_CAPTURE = 14,
    FLAG_PROMO_QUEEN_CAPTURE = 15
};    


// Move representation: 16 bits total
// Bits 0-5: from square (0-63)
// Bits 6-11: to square (0-63)
// Bits 12-15: flags (4 bit)

// --- FLAG LIST ---
// 0000 (0) = No flag
// 0001 (1) = Double Pawn Push
// 0010 (2) = Kingside Castle
// 0011 (3) = Queenside Castle
// 0100 (4) = Normal Capture
// 0101 (5) = EP Capture
// 1000 (8) = Promo Knight
// 1001 (9) = Promo Bishop
// 1010 (10)= Promo Rook
// 1011 (11)= Promo Queen
// 1100 (12)= Promo Knight + Capture
// 1101 (13)= Promo Bishop + Capture
// 1110 (14)= Promo Rook + Capture
// 1111 (15)= Promo Queen + Capture

// The reason why I chose this encoding:
// In all promotion types, 3rd bit is always 1. 
// And in all capture types, 2nd bit is always 1. 
// This allows for easy detection of quiet moves vs captures vs promotions with simple bit checks

using Move = uint16_t;

inline int move_flags(Move m) { return (m >> 12) & 0xF; } // Last 4 bits

inline int get_promotion_type(Move m) {
    // 8 = 1000 in binary, so we check if the 3rd bit is set to determine if it's a promotion
    // If it's a promotion, the type is determined by the last 2 (0011 -> 3 since 2^1 + 2^0) bits of the flags
    int promo = (move_flags(m) & 8) != 0 ? (move_flags(m) & 3) : -1;
    if (promo != -1) {
        switch (promo) {
            case 0: return KNIGHT; // 00
            case 1: return BISHOP; // 01
            case 2: return ROOK;   // 10
            case 3: return QUEEN;  // 11
        }
    }
    return -1; // Not a promotion
}

// Compare two moves for equality (from/to squares and promotion)
inline bool moves_equal(const Move& a, const Move& b) {
    return (a & 0x3F) == (b & 0x3F) && // from square
           ((a >> 6) & 0x3F) == ((b >> 6) & 0x3F) && // to square
              get_promotion_type(a) == get_promotion_type(b); // flags promotions only
}

// Move type helpers
inline bool is_quiet(const Move& m) {
    return move_flags(m) < 4; // No capture, no promotion 
}

inline bool is_capture(Move m) {
    return (move_flags(m) & 4) != 0; 
}

inline bool is_promotion(Move m) {
    return (move_flags(m) & 8) != 0; 
}

inline int move_from(Move m) { return m & 0x3F; } // First 6 bits
inline int move_to(Move m) { return (m >> 6) & 0x3F; } // Next 6 bits

inline Move create_move(int from, int to, int flags) {
    return Move((from & 63) | ((to & 63) << 6) | ((flags & 15) << 12)); 
}

#endif // MOVE_H