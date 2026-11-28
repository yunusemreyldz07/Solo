#include "nnue.h"
#include <cstring>
#include "nnue_data.h"
#include <iostream>
#include <array>
#include "board.h"
#include <algorithm>

constexpr int INPUT_SIZE = 768;
constexpr int HIDDEN_SIZE = 512;




// NNUE Weights and Biases
alignas(64) int16_t hiddenWeight[INPUT_SIZE * HIDDEN_SIZE]; 
alignas(64) int16_t hiddenBias[HIDDEN_SIZE];

alignas(64) int16_t outputWeight[HIDDEN_SIZE * 2]; 
int16_t outputBias;



// Optimization additions. 
// Instead of running a lot of loops while updating the accumulator, 
// we run a single loop that updates both accumulators at once and do add/sub in one loop.

void applyDirtyState(Accumulator* __restrict__ dst, const Accumulator* __restrict__ src, const DirtyState& dirty) {
    if (dirty.type == 0) {
        applyQuietBoth(dst, src, dirty.wAdd[0], dirty.wSub[0], dirty.bAdd[0], dirty.bSub[0]);
    } else if (dirty.type == 1) {
        applyCaptureBoth(dst, src, dirty.wAdd[0], dirty.wSub[0], dirty.wSub[1], dirty.bAdd[0], dirty.bSub[0], dirty.bSub[1]);
    } else if (dirty.type == 2) {
        applyCastlingBoth(dst, src, dirty.wAdd[0], dirty.wAdd[1], dirty.wSub[0], dirty.wSub[1], dirty.bAdd[0], dirty.bAdd[1], dirty.bSub[0], dirty.bSub[1]);
    }
}

// Quiet move or non-capture promotion 
// 1 add + 1 sub per perspective
void applyQuietBoth(Accumulator* __restrict__ dst, const Accumulator* __restrict__ src,
    int wAdd, int wSub,
    int bAdd, int bSub)
{
    const int16_t* __restrict__ addW = hiddenWeight + wAdd * HIDDEN_SIZE;
    const int16_t* __restrict__ subW = hiddenWeight + wSub * HIDDEN_SIZE;
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        dst[0][i] = src[0][i] + addW[i] - subW[i];
    }

    const int16_t* __restrict__ addB = hiddenWeight + bAdd * HIDDEN_SIZE;
    const int16_t* __restrict__ subB = hiddenWeight + bSub * HIDDEN_SIZE;
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        dst[1][i] = src[1][i] + addB[i] - subB[i];
    }
}

// Capture
// 1 add + 2 subs per perspective (moved piece arrives, moved piece leaves + captured piece removed)
void applyCaptureBoth(Accumulator* __restrict__ dst, const Accumulator* __restrict__ src,
    int wAdd, int wSub1, int wSub2,
    int bAdd, int bSub1, int bSub2)
{
    const int16_t* __restrict__ addW  = hiddenWeight + wAdd  * HIDDEN_SIZE;
    const int16_t* __restrict__ sub1W = hiddenWeight + wSub1 * HIDDEN_SIZE;
    const int16_t* __restrict__ sub2W = hiddenWeight + wSub2 * HIDDEN_SIZE;
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        dst[0][i] = src[0][i] + addW[i] - sub1W[i] - sub2W[i];
    }

    const int16_t* __restrict__ addB  = hiddenWeight + bAdd  * HIDDEN_SIZE;
    const int16_t* __restrict__ sub1B = hiddenWeight + bSub1 * HIDDEN_SIZE;
    const int16_t* __restrict__ sub2B = hiddenWeight + bSub2 * HIDDEN_SIZE;
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        dst[1][i] = src[1][i] + addB[i] - sub1B[i] - sub2B[i];
    }
}

// Castling
// 2 adds + 2 subs per perspective (king + rook each move)
void applyCastlingBoth(Accumulator* __restrict__ dst, const Accumulator* __restrict__ src,
    int wAdd1, int wAdd2, int wSub1, int wSub2,
    int bAdd1, int bAdd2, int bSub1, int bSub2)
{
    const int16_t* __restrict__ add1W = hiddenWeight + wAdd1 * HIDDEN_SIZE;
    const int16_t* __restrict__ add2W = hiddenWeight + wAdd2 * HIDDEN_SIZE;
    const int16_t* __restrict__ sub1W = hiddenWeight + wSub1 * HIDDEN_SIZE;
    const int16_t* __restrict__ sub2W = hiddenWeight + wSub2 * HIDDEN_SIZE;
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        dst[0][i] = src[0][i] + add1W[i] + add2W[i] - sub1W[i] - sub2W[i];
    }

    const int16_t* __restrict__ add1B = hiddenWeight + bAdd1 * HIDDEN_SIZE;
    const int16_t* __restrict__ add2B = hiddenWeight + bAdd2 * HIDDEN_SIZE;
    const int16_t* __restrict__ sub1B = hiddenWeight + bSub1 * HIDDEN_SIZE;
    const int16_t* __restrict__ sub2B = hiddenWeight + bSub2 * HIDDEN_SIZE;
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        dst[1][i] = src[1][i] + add1B[i] + add2B[i] - sub1B[i] - sub2B[i];
    }
}

// Legacy single accumulator update (kept for RefreshAccumulator)
void updateAccumulator(Accumulator& acc, int featureIdx, bool isAdd) { 
    int offset = featureIdx * HIDDEN_SIZE;
    
    if (isAdd) {
        for (int i = 0; i < HIDDEN_SIZE; i++) {
            acc[i] += hiddenWeight[offset + i];
        }
    } else {
        for (int i = 0; i < HIDDEN_SIZE; i++) {
            acc[i] -= hiddenWeight[offset + i];
        }
    }
}

int makeFeatureIndex(int piece_type, int piece_color, int square, int perspective) {
    // (if perspective is 0 it remains the same, if perspective is 1 it flips to the opposite color and square)
    int is_enemy = piece_color ^ perspective; 
    
    int pieceIdx = (piece_type - 1) + (is_enemy * 6);
    
    // if the perspective is zero (white), the square index remains the same. If the perspective is one (black), we flip the square index to mirror it vertically.
    int sq = square ^ (perspective * 56);

    return (pieceIdx * 64) + sq;
}

void load_nnue() {
    size_t offset = 0;

    std::memcpy(hiddenWeight, nnue_data + offset, sizeof(hiddenWeight));
    offset += sizeof(hiddenWeight);

    std::memcpy(hiddenBias, nnue_data + offset, sizeof(hiddenBias));
    offset += sizeof(hiddenBias);

    std::memcpy(outputWeight, nnue_data + offset, sizeof(outputWeight));
    offset += sizeof(outputWeight);

    std::memcpy(&outputBias, nnue_data + offset, sizeof(outputBias));
}

void RefreshAccumulator(const Board& board, Accumulator* acc_white, Accumulator* acc_black) {
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        (*acc_white)[i] = hiddenBias[i];
        (*acc_black)[i] = hiddenBias[i];
    }

    for (int sq = 0; sq < 64; sq++) {
        int piece = board.mailbox[sq];
        
        if (piece != 0) {
            int p_color = (piece - 1) / 6; 
            int p_type  = ((piece - 1) % 6) + 1; 

            int white_idx = makeFeatureIndex(p_type, p_color, sq, WHITE); // WHITE = 0
            updateAccumulator(*acc_white, white_idx, true);

            int black_idx = makeFeatureIndex(p_type, p_color, sq, BLACK); // BLACK = 1
            updateAccumulator(*acc_black, black_idx, true);
        }
    }
}

int evaluate_nnue(const Accumulator& acc_white, const Accumulator& acc_black, int side_to_move) {
    const int16_t* __restrict__ us   = (side_to_move == WHITE) ? acc_white.data() : acc_black.data();
    const int16_t* __restrict__ them = (side_to_move == WHITE) ? acc_black.data() : acc_white.data();
    const int16_t* __restrict__ w_us   = outputWeight;
    const int16_t* __restrict__ w_them = outputWeight + HIDDEN_SIZE;

    int32_t sum_us = 0;
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        int32_t v = std::clamp<int16_t>(us[i], 0, 255);
        sum_us += (v * v) * w_us[i];
    }

    int32_t sum_them = 0;
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        int32_t v = std::clamp<int16_t>(them[i], 0, 255);
        sum_them += (v * v) * w_them[i];
    }

    // Turning it into a real chess score (QA=255, QB=64, Scale=400)
    int32_t raw_sum = sum_us + sum_them;
    return ((raw_sum / 255) + outputBias) * 400 / 16320; // 255 * 64 = 16320
}