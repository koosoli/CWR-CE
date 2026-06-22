#include <Poseidon/AI/AI.hpp>
#include <Poseidon/AI/ArcadeTemplate.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Poseidon;
TEST_CASE("arcadeTemplate compiles", "[ai]")
{
    REQUIRE(sizeof(ArcadeTemplate) > 0);
}

TEST_CASE("ExperienceForDestroyedCost covers the top-bucket band (no zero gap)", "[ai][experience]")
{
    // Costs map to the first bucket whose maxCost they fit under; the last entry
    // is the catch-all for anything above the second-to-last bucket. The pre-fix
    // AISubgroup lookup tested `cost > maxCost[n-1]` for the catch-all and looped
    // only to n-2, so a cost in (maxCost[n-2], maxCost[n-1]] fell through to 0 --
    // the AI_ERROR(base > 0) the headless combat stress run tripped.
    AutoArray<ExperienceDestroyInfo> saved = ExperienceDestroyTable;
    ExperienceDestroyTable.Resize(3);
    ExperienceDestroyTable[0].maxCost = 100.0f;
    ExperienceDestroyTable[0].exp = 1.0f;
    ExperienceDestroyTable[1].maxCost = 1000.0f;
    ExperienceDestroyTable[1].exp = 2.0f;
    ExperienceDestroyTable[2].maxCost = 5000.0f;
    ExperienceDestroyTable[2].exp = 3.0f; // catch-all

    REQUIRE(ExperienceForDestroyedCost(0.0f) == 1.0f);   // tiny -> first bucket
    REQUIRE(ExperienceForDestroyedCost(50.0f) == 1.0f);  // first bucket
    REQUIRE(ExperienceForDestroyedCost(500.0f) == 2.0f); // middle bucket
    // The regression: a cost between the last two buckets. Pre-fix this returned 0.
    REQUIRE(ExperienceForDestroyedCost(3000.0f) == 3.0f);
    REQUIRE(ExperienceForDestroyedCost(99999.0f) == 3.0f); // far above -> catch-all

    ExperienceDestroyTable = saved;
}
