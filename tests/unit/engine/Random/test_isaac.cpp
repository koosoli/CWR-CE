#include <catch2/catch_test_macros.hpp>
#include <Random/isaac.hpp>

// ============================================================================
// Random/common testing - additional tests
// ============================================================================
// Tests for modules not yet covered:
// - ISAAC RNG (isaac.hpp)
// NOTE: enumNames and randomGen are already comprehensively tested
// (20 existing tests with 867 assertions)
//
// Coverage areas:
// 1. ISAAC RNG (4 tests)
//
// Total: 4 new tests to complement existing 20 tests
// ============================================================================

// ============================================================================
// Section 1: ISAAC RNG Tests (4 tests)
// ============================================================================

TEST_CASE("ISAAC - Construction and seeding", "[common][isaac][random]")
{
    SECTION("32-bit ISAAC construction")
    {
        QTIsaac<8, ISAAC_INT> rng(123, 456, 789);

        // Should generate values
        ISAAC_INT val = rng.rand();
        REQUIRE(val != 0); // Extremely unlikely to be zero
    }

    SECTION("Multiple values are different")
    {
        QTIsaac<8, ISAAC_INT> rng(111, 222, 333);

        ISAAC_INT val1 = rng.rand();
        ISAAC_INT val2 = rng.rand();
        ISAAC_INT val3 = rng.rand();

        // Values should be different (extremely unlikely to repeat)
        REQUIRE(val1 != val2);
        REQUIRE(val2 != val3);
        REQUIRE(val1 != val3);
    }
}

TEST_CASE("ISAAC - Deterministic behavior", "[common][isaac][random]")
{
    SECTION("Same seed produces same sequence")
    {
        QTIsaac<8, ISAAC_INT> rng1(100, 200, 300);
        QTIsaac<8, ISAAC_INT> rng2(100, 200, 300);

        // Generate 100 values from both - should be identical
        for (int i = 0; i < 100; i++)
        {
            ISAAC_INT val1 = rng1.rand();
            ISAAC_INT val2 = rng2.rand();
            REQUIRE(val1 == val2);
        }
    }

    SECTION("Different seeds produce different sequences")
    {
        QTIsaac<8, ISAAC_INT> rng1(100, 200, 300);
        QTIsaac<8, ISAAC_INT> rng2(101, 200, 300); // Different first seed

        // First values should be different
        ISAAC_INT val1 = rng1.rand();
        ISAAC_INT val2 = rng2.rand();

        REQUIRE(val1 != val2);
    }
}

TEST_CASE("ISAAC - Re-seeding", "[common][isaac][random]")
{
    SECTION("srand() resets sequence")
    {
        QTIsaac<8, ISAAC_INT> rng(123, 456, 789);

        // Generate some values
        ISAAC_INT initial1 = rng.rand();
        ISAAC_INT initial2 = rng.rand();
        rng.rand(); // Advance further

        // Re-seed with same values
        rng.srand(123, 456, 789);

        // Should get same initial sequence
        ISAAC_INT after1 = rng.rand();
        ISAAC_INT after2 = rng.rand();

        REQUIRE(initial1 == after1);
        REQUIRE(initial2 == after2);
    }
}

TEST_CASE("ISAAC - Different ALPHA values", "[common][isaac][random]")
{
    SECTION("ALPHA=4 (smaller state)")
    {
        QTIsaac<4, ISAAC_INT> rng(1, 2, 3);

        // Should generate values
        ISAAC_INT val = rng.rand();
        REQUIRE(val != 0);
    }

    SECTION("ALPHA=8 (default)")
    {
        QTIsaac<8, ISAAC_INT> rng(1, 2, 3);

        // Should generate values
        ISAAC_INT val = rng.rand();
        REQUIRE(val != 0);
    }
}

// ============================================================================
// ============================================================================
// Summary: ISAAC Additional Tests Complete
// ============================================================================
//
// Tests: 4 new tests
//
// Sections:
// - ISAAC RNG: 4 tests (construction, determinism, re-seed, ALPHA variations)
// ============================================================================
