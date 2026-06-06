# Solo - UCI Chess Engine
**Version 2.2.0**

A bitboard-based chess engine with advanced search techniques and a neural network evaluation.

### Note:
Aggression was tested in earlier commits but was not released. I am planning to turn the engine into an aggressive engine when I got the NNUE done perfectly with input/output buckets and 1024HL. Currently, it uses 512HL net trained with self-play data.

## Features

- **Bitboard Representation**: 64-bit bitboards for efficient move generation
- **Magic Bitboards**: Fast sliding piece attack generation using magic numbers

- **Search Algorithm**:
  - Negamax with alpha-beta pruning
  - Iterative deepening
  - Aspiration windows
  - Principal Variation Search (PVS)
  - Null Move Pruning (NMP) with verification search and TT-adjusted margin
  - Late Move Reductions (LMR)
  - Late Move Pruning (LMP) with improving heuristic
  - Reverse Futility Pruning (RFP) with improving heuristic
  - Futility Pruning (FP)
  - Razoring
  - Static Exchange Evaluation (SEE) pruning
  - Internal Iterative Reductions (IIR)
  - Check extensions
  - Singular Extensions (SE) with Multicut and Negative Extensions
  - History Pruning
  - Quiescence search with SEE filtering
  - Repetition / draw detection in search
  - Soft/Hard time management with best-move stability
  - Transposition Table

- **Move Ordering**:
  - Staged move generation via MovePicker (lazy generation)
  - TT move first
  - Good captures (SEE ≥ threshold) with MVV-LVA scoring
  - Killer moves
  - History + continuation history (1, 2, 4 ply) for quiet moves (Bad quiets are penalized)
  - Bad captures ordered last

- **Evaluation**:
  - 512HL NNUE with lazy accumulator updates, trained on ~1 billion self-play generated positions.

## Building

The Makefile automatically detects your operating system and compiles with the highest optimizations available for your processor architecture.
```bash
# Simply run make
make

# Clean build artifacts
make clean
```

**Output**: Executable will be created as `Solo.exe` (Windows) or `Solo` (Linux/Mac)

### Manual Compilation

If you don't have Make:

**Windows (MinGW/MSYS2)**
```bash
g++ -O3 -flto -march=native -std=c++23 -ffast-math -pthread main.cpp board.cpp movegen.cpp movepicker.cpp search.cpp evaluation.cpp bitboard.cpp history.cpp nnue.cpp datagen.cpp -o Solo.exe -static -static-libgcc -static-libstdc++
```

**Linux**
```bash
g++ -O3 -flto -march=native -std=c++23 -ffast-math -pthread main.cpp board.cpp movegen.cpp movepicker.cpp search.cpp evaluation.cpp bitboard.cpp history.cpp nnue.cpp datagen.cpp -o Solo -lm
```

**macOS (Apple Silicon)**
```bash
clang++ -O3 -flto -march=native -std=c++23 -ffast-math -pthread main.cpp board.cpp movegen.cpp movepicker.cpp search.cpp evaluation.cpp bitboard.cpp history.cpp nnue.cpp datagen.cpp -o Solo -lm
```

## Usage

### UCI Mode
```bash
./Solo

# Example UCI commands:
uci
setoption name Hash value 64
position startpos moves e2e4 e7e5
go depth 10
```

### Benchmark
```bash
./Solo bench
```

Runs a built-in benchmark on 12 positions at depth 8.

## UCI Options

| Option | Type | Default | Range | Description |
|--------|------|---------|-------|-------------|
| `Hash` | spin | 128 | 1-2048 | Transposition table size in MB |
| `Threads` | spin | 1 | 1-8 | Number of search threads *(not implemented yet)* |
| `Use_NNUE` | check | true | true/false | Toggle between NNUE and classical HCE evaluation |

## Strength

- **CCRL ELO**: [~3100+](https://computerchess.org.uk/404/cgi/engine_details.cgi?print=Details&each_game=1&eng=Solo%202.1.0%2064-bit#Solo_2_1_0_64-bit)
- **Lichess ELO**: [~2600](https://lichess.org/@/SoloBot)

## Roadmap

- [ ] Multi-threading support (Lazy SMP)
- [ ] Correction history
- [ ] Better NNUE net with aggressiveness (input/output buckets, 1024HL)

## Project Structure
```
├── bitboard.cpp/h      # Magic bitboards & attack generation
├── board.cpp/h         # Board representation & move make/unmake
├── evaluation.cpp/h    # Tuned tapered evaluation & mobility
├── history.cpp/h       # History, continuation history
├── move.h              # Move encoding, flags & helpers
├── movegen.cpp         # Move generation
├── movepicker.cpp/h    # Staged move picker (lazy generation)
├── piece.h             # Piece types, colors & helpers
├── search.cpp/h        # Negamax search, pruning & reductions
├── uci.cpp/h           # UCI protocol handler
├── types.h             # Basic types & constants
├── nnue.cpp/h          # NNUE evaluation (512 hidden layer)
├── datagen.cpp/h       # Self-play data generation for training
├── main.cpp            # Entry point
└── Makefile            # Build system
```

## Credits

- **Author**: xsolod3v
- **Inspired by**: Patricia engine by Adam Kulju, Potential Engine by ProgramciDusunur
- **Evaluation**: For HCE: PeSTO piece-square tables by Ronald Friederich (further tuned). For NNUE: self-play generated data.
- **Resources**: Chess Programming Wiki, Potential source code, Ethereal source code, Patricia wiki

## License

**MIT License** - Do whatever you want with it!

See [LICENSE](LICENSE) for full details.

---

**Want to contribute?** Feel free to open issues or pull requests!