#ifndef BOARD_H
#define BOARD_H

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "nnue.h"
#include "types.h"
#include "piece.h"
#include "move.h"

// Search constants
inline constexpr int MAX_GAME_PLY = 2048;
inline constexpr int MAX_PLY = 128;
inline constexpr int16_t MATE_SCORE = 30000;
inline constexpr int16_t VALUE_INF  = 32000;

inline constexpr uint8_t CASTLE_WK = 1; // White Kingside (0001)
inline constexpr uint8_t CASTLE_WQ = 2; // White Queenside (0010)
inline constexpr uint8_t CASTLE_BK = 4; // Black Kingside (0100)
inline constexpr uint8_t CASTLE_BQ = 8; // Black Queenside (1000)

struct UndoState {
    int capturedPiece;
    uint8_t castling;
    int8_t enPassant;
    int16_t halfMoveClock;
    uint64_t hash;
    DirtyState dirty;
};

struct Board {
    Bitboard piece[6];
    Bitboard color[2];
    uint64_t hash;

    uint8_t castling;
    int8_t enPassant;
    uint8_t stm;

    std::vector<Move> moveHistory;
    std::vector<UndoState> undoStack;

    int8_t mailbox[64];
    int16_t halfMoveClock;

    std::unique_ptr<std::array<Accumulator, 2>[]> accStack;
    std::unique_ptr<bool[]> accValid;

    Board();
    Board(const Board& other);
    Board& operator=(const Board& other);
    void reset();
    void loadFEN(const std::string& fen);
    void makeMove(Move move);
    void unmakeMove(Move move);
    void ensureAccumulator(int ply);
};

inline int row_col_to_sq(int row, int col) {
    return (7 - row) * 8 + col;
}

inline int sq_to_row(int sq) {
    return 7 - (sq / 8);
}

inline int sq_to_col(int sq) {
    return sq % 8;
}

inline int piece_at_sq(const Board& board, int sq) {
    return board.mailbox[sq];
}

inline int side_to_move(const Board& b) { return b.stm; }
inline int opponent(const Board& b)     { return other_color(b.stm); }

inline void king_square(const Board& board, bool white, int& outSq) {
    Bitboard k = board.piece[KING - 1] & board.color[white ? WHITE : BLACK];
    if (!k) { outSq = -1; return; }
    outSq = lsb(k);
}

// Move generation
void get_all_moves(Board& board, Move moves[], int& moveCount);
void get_capture_moves(Board& board, Move moves[], int& moveCount);

// Attack detection
bool is_square_attacked(const Board& board, int sq, bool isWhiteAttacker);

int staticExchangeEvaluation(const Board& board, const Move& move, int threshold);

// Pseudo-legal move generation
void generate_pseudo_moves(const Board& board, Move moves[], int& moveCount);
void generate_pseudo_captures(const Board& board, Move moves[], int& moveCount);
void generate_pseudo_quiets(const Board& board, Move moves[], int& moveCount);
bool is_move_pseudo_legal(const Board& board, Move move);

// Utility
void printBoard(const Board& board);
Move uci_to_move(const std::string& uci, const Board& board);

// Zobrist hashing
struct Zobrist {
    uint64_t piece[12][64]{};
    uint64_t castling[16]{};
    uint64_t epFile[9]{};
    uint64_t side{};

    static uint64_t splitmix64(uint64_t& x);
    Zobrist();
};

const Zobrist& zobrist();
int piece_to_zobrist_index(int piece);
uint64_t position_key(const Board& board);
bool is_repetition(const std::vector<uint64_t>& positionHistory, int16_t halfMoveClock);

inline bool is_fifty_move_draw(const Board& board) {
    return board.halfMoveClock >= 100;
}

bool is_insufficient_material(const Board& board);

#endif
