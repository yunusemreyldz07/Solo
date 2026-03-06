#include "evaluation.h"
#include <cmath>
#include "board.h"
#include "bitboard.h"
#include <iostream>

// Bit manipulation - using inline functions from types.h
// popcount() and lsb() are defined there
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define pop_bit(bitboard, square) (get_bit(bitboard, square) ? (bitboard) ^= (1ULL << (square)) : 0)

namespace {

constexpr int OTHER(int side) { return side ^ 1; }

// Vertical flip for A8=0 .. H1=63 indexing.
constexpr int mirror_sq(int sq) { return sq ^ 56; }
}


const int mg_value[6] = { 82, 337, 365, 477, 1025, 0 };
const int eg_value[6] = { 94, 281, 297, 512,  936, 0 };

const int gamephaseInc[6] = { 0, 1, 1, 2, 4, 0 };

// Mobility bonuses and penalties
// Can go to 8 squares max
// 0 squares = -20
// 4 squares = 0
// 8 squares = +15
const int KnightMobility[9] = { -20, -10, -5, -2, 0, 5, 10, 12, 15 };

// Can go to 13 squares max
// Same logic here, hopefully this will prevent bad BISHOP placements lol
// 0 squares = -20
// 6 squares = 0
// 13 squares = +20
const int BishopMobility[14] = { -20, -10, -5, -2, 0, 2, 4, 6, 8, 10, 12, 15, 18, 20 };

// and the rest...
const int RookMobility[15] = { -10, -5, -2, 0, 2, 4, 6, 8, 10, 12, 14, 16, 20, 25, 30 };

// QUEEN bonuses are not that much according to other pieces, it is because QUEEN already can go to many squares and this might cause our engine to get its QUEEN out too early
const int QueenMobility[28] = { -5, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15 };

// Pesto tables
const int mg_pawn_table[64] = {
       0,   0,   0,   0,   0,   0,   0,   0,
      422,  222,  381,  116,  163, -173, -567, -100,
      -48,  105,   59,    4,   -2,  -89,  190, -158,
      -42,   39,  -20,  -23,  -58,   33,  -89,  -50,
      -41,    4,  -27,   10,    4,   -6,  -19,  -60,
      -33,   -3,  -62,  -35,   -4,   27,   27,  -26,
      -38,  -24,  -63,    6,   -4,   23,   42,  -34,
       0,   0,   0,   0,   0,   0,   0,   0
};
const int eg_pawn_table[64] = {
     0, 0, 0, 0, 0, 0, 0, 0,
       24,  133,   37,  169,   63,  104,  192,  128,
       78,   42,   -1,   91,   -4,   -7,  -20,   42,
       -2,  -25,  -47,   28,  -32,  -98,  -51,  -54,
      -40,  -17,  -73,   -4,  -59,  -79,  -50,  -79,
      -78,  -24,  -72,    4,  -54,  -45,  -84,  -63,
      -58,  -24,  -57,  392,  -28,  -46,  -68,  -71,
     0, 0, 0, 0, 0, 0, 0, 0
};


const int mg_knight_table[64] = {
     -327,  204, -145, -178,   62,  207,  346, -297,
      152,  146,  -66,  114,   49,  -31, -154,   49,
       78,    5,   18,  143,   41,  328,   -8,  284,
     -111,   -6,  152,  -23,   23,   60,   22,  151,
       40, -100,   28,   32,    6,   36,    8,  -13,
      -18,  -37,  -20,   31, -130,   -2,   25, -231,
     -346,  -23, -104,    2,  -24,  -11,   62,  -87,
      223,  -24, -572, -518,  -57,   24,  -33,  525
};
const int eg_knight_table[64] = {
       45, -137,  117,  148,  108, -105,  -47,  270,
        3,  -94,    4,   12,   35,   35,  170,  103,
     -195,   95,   75,   -1,  -15,   15,   83,  -46,
      -22,   32,  -13,   50,   58,   74,  -24, -159,
      -16,   57,  -22,   29,   51,   21,   31,   36,
      -66,  -35,  -21,   51,   46,   15,  -88, -175,
      -17, -113,  -25,  -27,  -46,  -17,   -1,  -74,
      -47,   -6,  128,  -14,   -5,  -45,  -84, -221
};


const int mg_bishop_table[64] = {
      391,  -85, -107, -155, -310,   -5,  -48,  650,
     -197, -114,   51,  -70,  -32, -282, -253, -152,
       33,  145,  -33,  -18,  152,  -49,    1,   93,
      -89,  -10,    6,   89,    2,  -29,   17,  -26,
       -9,  -23,    9,    9,  -17,   13,   10,   44,
      -76,   60,  -47,   24,  -30,   95,   58,  223,
      324,   13,   33,  -16,   12,   18,  -16,   23,
     -348,  121,   -5, -229,   58,    1, -204,  297
};
const int eg_bishop_table[64] = {
       15,   80, -158,   11,   14,  -22,  -39,  -17,
       41, -107,  -29,   86,   67,  145,   43,  186,
      -40,    2,   28,   -6,  -47,   73,   11,  -12,
       43,   76,   34,   23,   31,   25,  -85,  -92,
       20,   21,    7,   11,  153,  -55,  -26,  -59,
        7,  -35,   19,   60,   58,   17,  -70, -146,
     -189,  -45,   94,   19,    8, -105,  146, -101,
      212,  153,  -86,  130,  -74,   21, -157, -380
};


const int mg_rook_table[64] = {
       43,   30,   13,   36,   53,  480,  421,  -68,
      -52,   40,   59,   45,   90,  174,  289,   30,
      -18, -129,  -12,   49,  183,  315, -127,  -93,
     -176,  -74,   24,   45,   62,  140, -141, -127,
     -122, -111,   18,  -33,  -56,  -59,    5,  -90,
     -311,  -91,  123,  -79,  -75,  -20,  -12, -130,
     -185,  -37,  -57,    6,  -47,  -73, -174,  265,
      -57,  -65,  -33,  -34,  -49,  -50,   35,  -13
};
const int eg_rook_table[64] = {
       52,   28,   48,   55,   16,  -80,  -27,   98,
       84,   44,   57,   38,   57,  -44,  -27,   47,
       65,   84,   44,   37,   -8,  -60,   82,   66,
       97,   51,   29,   18,  -47,  -23,   52,   42,
       30,   75,  -19,  -27,  -15,  -12,   33,   16,
       42,  -18,  -28,  -33,  -42,  -62,  -86,  -77,
      -19,  -84,  -22,  -70,  -66,  -12,  -74, -113,
      -28,    0,   -9,   -1,  -34,  -29,  -69,  -95
};

const int mg_queen_table[64] = {
     -120,  -43,  -81, -135,  312,    0,  510,  -72,
      -62, -124,  -79,   66,   46,  194,  -14,  -35,
      -52,  -35,  -23, -155,   11,  -51,  178, -144,
        7,   31,  -18,  -73,   31,  -83,    1,  -32,
       -3,   28,    5,    8,  -36,  -17,  -63, -116,
       79,   35,  -77,   33,   13,   -3,  -42,  -48,
      -73,   10,   54,   42,   35,  -11,   48,    8,
      107,  -34,   10,   39,    8,  -88,  272, -144
};
const int eg_queen_table[64] = {
      132,  123,  157,  225,  -91,  128, -172,  129,
      145,  233,   98,   46,   14,   31,  151,   48,
      153,   92,   99,  188,  149,   35,    5,  196,
       17,  -89,   22,   53,    6,  169,   87,   36,
      -31,  -85,  -15,   38,   75,    1,  114,   85,
     -131, -101,    6,  -64,  -73,   36,   36, -128,
      -69, -135, -134, -104,  -65, -102, -227, -167,
     -325, -179,  -96, -149, -226,   21, -310, -121
};


const int mg_king_table[64] = {
      119,  301, -374,  193,  371,   77,  -27,  282,
      148,   -3,   18,  -17, -127,   93,  101, -287,
      -88, -268, -413, -281, -438,   44,  255,   67,
      162, -247, -459, -326, -251,   15,  -35, -239,
     -212,  373,  180,   52,  -93,  150,  -57,   91,
     -154,  244,  197,  120,   14,  152,   88,   49,
     -265,   90, -153,   54,   85,   39,  160,  142,
     -446,  163,   15,  144,  109,  -21,  159,  190
};
const int eg_king_table[64] = {
       15,   64,  105,   66,  -40,  -11,  -37, -110,
       35,   65,   93,   92,   58,   -2,  -18,   60,
       77,  174,  232,  160,  145,  -38, -120,  -23,
      -23,  127,  150,  118,   79,    8,   28,  -38,
      -37,  -36,  -20,   22,   20,  -26,    4,  -93,
      -58, -132,  -57,  -45,   17,  -38,  -64,  -77,
        0,    8,  -25,  -62,  -52,  -35,  -52, -113,
      211,  -47,  -50, -121, -128,  -66, -116, -214
};

// Pointer arrays for easy access
const int* mg_pesto_tables[6] = { mg_pawn_table, mg_knight_table, mg_bishop_table, mg_rook_table, mg_queen_table, mg_king_table };
const int* eg_pesto_tables[6] = { eg_pawn_table, eg_knight_table, eg_bishop_table, eg_rook_table, eg_queen_table, eg_king_table };

namespace {
int mg_table[12][64];
int eg_table[12][64];
bool tables_initialized = false;

int piece_to_table_index(int piece) {
    const int absPiece = piece > 0 ? piece : -piece;
    if (absPiece < 1 || absPiece > 6) return -1;
    return absPiece - 1;
}

void init_tables() {
    for (int p = 0; p < 6; ++p) {
        const int wIdx = p * 2;
        const int bIdx = p * 2 + 1;
        for (int sq = 0; sq < 64; ++sq) {
            const int msq = mirror_sq(sq);
            // PSTs are A8..H1; board squares are A1..H8.
            // White uses mirrored squares, black uses raw squares.
            mg_table[wIdx][sq] = mg_value[p] + mg_pesto_tables[p][msq];
            eg_table[wIdx][sq] = eg_value[p] + eg_pesto_tables[p][msq];
            mg_table[bIdx][sq] = mg_value[p] + mg_pesto_tables[p][sq];
            eg_table[bIdx][sq] = eg_value[p] + eg_pesto_tables[p][sq];
        }
    }
}

void ensure_tables_init() {
    if (!tables_initialized) {
        init_tables();
        tables_initialized = true;
        }
    }
}

const int doublePawnPenaltyOpening = -5;
const int doublePawnPenaltyEndgame = -49;

int evaluate_mobility(const Board& board, int pieceType, bool isWhite, Bitboard occupy) {
    Bitboard myPieces = isWhite ? board.color[WHITE] : board.color[BLACK];
    
    Bitboard pieces = board.piece[pieceType - 1] & myPieces;
    int totalMobility = 0;

    while (pieces) {
        int sq = lsb(pieces);
        pop_bit(pieces, sq);
        
        Bitboard attacks = 0ULL;

        switch (pieceType) {
            case KNIGHT:
                attacks = knight_attacks[sq];
                break;
            case BISHOP:
                attacks = get_bishop_attacks(sq, occupy);
                break;
            case ROOK:
                attacks = get_rook_attacks(sq, occupy);
                break;
            case QUEEN:
                attacks = get_bishop_attacks(sq, occupy) | get_rook_attacks(sq, occupy);
                break;
        }

        Bitboard validMoves = attacks & ~myPieces;
        
        int mobilityCount = popcount(validMoves);

        if (pieceType == KNIGHT) totalMobility += KnightMobility[mobilityCount];
        else if (pieceType == BISHOP) totalMobility += BishopMobility[mobilityCount];
        else if (pieceType == ROOK) totalMobility += RookMobility[mobilityCount];
        else if (pieceType == QUEEN) totalMobility += QueenMobility[mobilityCount];
    }
    
    return totalMobility;
}

int evaluate_board(const Board& board) {
    ensure_tables_init();

    int mg[2] = {0, 0};
    int eg[2] = {0, 0};
    int gamePhase = 0;
    int side2move = board.stm == WHITE ? WHITE : BLACK;

    for (int color = 0; color < 2; color++) {
        for (int p = 0; p < 6; p++) {
            Bitboard bb = board.piece[p] & board.color[color];
            const int tableIdx = p * 2 + (color == BLACK ? 1 : 0);
            while (bb) {
                int sq = lsb(bb);
                bb &= bb - 1;
                mg[color] += mg_table[tableIdx][sq];
                eg[color] += eg_table[tableIdx][sq];
                gamePhase += gamephaseInc[p];
            }
        }
    }

    // Double pawn penalty
    Bitboard file_masks[8] = {
        0x0101010101010101ULL, // File A
        0x0202020202020202ULL, // File B
        0x0404040404040404ULL, // File C
        0x0808080808080808ULL, // File D
        0x1010101010101010ULL, // File E
        0x2020202020202020ULL, // File F
        0x4040404040404040ULL, // File G
        0x8080808080808080ULL  // File H
    };

    for (int color = 0; color < 2; color++) {
        Bitboard pawns = board.piece[PAWN - 1] & board.color[color];
        for (int file = 0; file < 8; file++) {
            int count = popcount(pawns & file_masks[file]);
            if (count > 1) {
                mg[color] += doublePawnPenaltyOpening * (count - 1);
                eg[color] += doublePawnPenaltyEndgame * (count - 1);
            }
        }
    }

    /* tapered eval */
    int mgScore = mg[side2move] - mg[OTHER(side2move)];
    int egScore = eg[side2move] - eg[OTHER(side2move)];
    
    int mgPhase = gamePhase;
    if (mgPhase > 24) mgPhase = 24; 
    
    int egPhase = 24 - mgPhase;
    
    int staticEval = (mgScore * mgPhase + egScore * egPhase) / 24;

    // Mobility evaluation
    int mobilityScore = 0;
    Bitboard occupy = board.color[WHITE] | board.color[BLACK];
    for (int pieceType = KNIGHT; pieceType <= QUEEN; pieceType++) {
        mobilityScore += evaluate_mobility(board, pieceType, board.stm == WHITE, occupy);
        mobilityScore -= evaluate_mobility(board, pieceType, !(board.stm == WHITE), occupy);
    }

    return (staticEval + mobilityScore);
}

