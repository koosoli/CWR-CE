#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

enum class TetrisPieceType : uint8_t {
    I,
    O,
    T,
    L,
    J,
    S,
    Z
};

struct TetrisCellPosition {
    int32_t x = 0;
    int32_t y = 0;
};

struct TetrisActivePiece {
    TetrisPieceType type = TetrisPieceType::I;
    int32_t rotation = 0;
    int32_t x = 0;
    int32_t y = 0;
};

class TetrisGame {
public:
    static constexpr int32_t BoardWidth = 10;
    static constexpr int32_t BoardHeight = 20;

    TetrisGame();

    void Reset();

    bool AdvanceGravityStep();
    bool SoftDrop();
    void MoveLeft();
    void MoveRight();
    void RotateClockwise();

    [[nodiscard]] bool IsCellOccupied(int32_t x, int32_t y) const;
    [[nodiscard]] bool IsGameOver() const;
    [[nodiscard]] int32_t Score() const;
    [[nodiscard]] int32_t LinesCleared() const;
    [[nodiscard]] const TetrisActivePiece& ActivePiece() const;
    [[nodiscard]] TetrisPieceType NextPieceType() const;
    [[nodiscard]] std::array<TetrisCellPosition, 4> ActivePieceBlocks() const;

    void ClearBoard();
    void SetOccupied(int32_t x, int32_t y, bool occupied);
    void ForceActivePiece(TetrisPieceType type, int32_t rotation, int32_t x, int32_t y);

private:
    using BoardCells = std::array<uint8_t, BoardWidth * BoardHeight>;

    static std::size_t CellIndex(int32_t x, int32_t y);

    [[nodiscard]] bool Collides(const TetrisActivePiece& piece) const;
    void LockActivePiece();
    void ClearCompletedLines();
    void SpawnNextPiece();

    BoardCells _board {};
    TetrisActivePiece _activePiece {};
    std::array<TetrisPieceType, 7> _pieceSequence {};
    std::size_t _nextSequenceIndex = 0;
    int32_t _score = 0;
    int32_t _linesCleared = 0;
    bool _gameOver = false;
};
