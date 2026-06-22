#include <catch2/catch_test_macros.hpp>

#include "TetrisGame.hpp"
#include <stdint.h>

TEST_CASE("TetrisGame starts with a deterministic opening piece", "[tetris][game]")
{
    TetrisGame game;

    CHECK(game.ActivePiece().type == TetrisPieceType::I);
    CHECK(game.NextPieceType() == TetrisPieceType::O);
    CHECK(game.Score() == 0);
    CHECK(game.LinesCleared() == 0);
}

TEST_CASE("TetrisGame stops horizontal movement at the playfield wall", "[tetris][game]")
{
    TetrisGame game;

    game.ForceActivePiece(TetrisPieceType::O, 0, -1, 0);
    game.MoveLeft();

    CHECK(game.ActivePiece().x == -1);

    game.MoveRight();
    CHECK(game.ActivePiece().x == 0);
}

TEST_CASE("TetrisGame locks pieces and clears completed lines deterministically", "[tetris][game]")
{
    TetrisGame game;
    game.ClearBoard();

    for (int32_t x = 4; x < TetrisGame::BoardWidth; ++x)
    {
        game.SetOccupied(x, TetrisGame::BoardHeight - 1, true);
    }

    game.ForceActivePiece(TetrisPieceType::I, 0, 0, TetrisGame::BoardHeight - 2);
    CHECK(game.AdvanceGravityStep());

    CHECK(game.LinesCleared() == 1);
    CHECK(game.Score() == 100);
    for (int32_t x = 0; x < TetrisGame::BoardWidth; ++x)
    {
        CHECK_FALSE(game.IsCellOccupied(x, TetrisGame::BoardHeight - 1));
    }
}

TEST_CASE("TetrisGame refuses rotations that would leave the playfield", "[tetris][game]")
{
    TetrisGame game;
    game.SetOccupied(5, 1, true);
    game.ForceActivePiece(TetrisPieceType::T, 0, 3, 0);

    game.RotateClockwise();

    CHECK(game.ActivePiece().rotation == 0);
}
