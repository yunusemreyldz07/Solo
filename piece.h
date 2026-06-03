#ifndef PIECE_H
#define PIECE_H
// Piece constants
// Convention: empty = 0, white pieces = 1-6, black pieces = 7-12
// Piece type = ((piece - 1) % 6) + 1 for non-empty pieces
// Color = (piece >= 7) ? BLACK : WHITE for non-empty pieces
inline constexpr int EMPTY = 0;

// Colors
inline constexpr int WHITE = 0;
inline constexpr int BLACK = 1;

// White pieces (1-6)
inline constexpr int W_PAWN   = 1;
inline constexpr int W_KNIGHT = 2;
inline constexpr int W_BISHOP = 3;
inline constexpr int W_ROOK   = 4;
inline constexpr int W_QUEEN  = 5;
inline constexpr int W_KING   = 6;

// Black pieces (7-12)
inline constexpr int B_PAWN   = 7;
inline constexpr int B_KNIGHT = 8;
inline constexpr int B_BISHOP = 9;
inline constexpr int B_ROOK   = 10;
inline constexpr int B_QUEEN  = 11;
inline constexpr int B_KING   = 12;

// Piece types (for indexing, color-agnostic)
inline constexpr int PAWN   = 1;
inline constexpr int KNIGHT = 2;
inline constexpr int BISHOP = 3;
inline constexpr int ROOK   = 4;
inline constexpr int QUEEN  = 5;
inline constexpr int KING   = 6;

// Helper functions for piece encoding
inline constexpr int piece_type(int piece) {
    return (piece == 0) ? 0 : ((piece - 1) % 6) + 1;
}

inline constexpr int piece_color(int piece) {
    if (piece >= 7) {
        return BLACK;
    }
    return piece != 0 ? WHITE : -1; // Return -1 for empty squares
}

inline constexpr int make_piece(int type, int color) {
    return (color == WHITE) ? type : type + 6;
}

// Color helper
inline constexpr uint8_t other_color(uint8_t c) { return c ^ 1; }

#endif // PIECE_H