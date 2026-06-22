#include <catch2/catch_test_macros.hpp>
#include "../../apps/tools/Studio/FileCategory.hpp"
#include <string>

using Poseidon::CategorizeExtension;
using Poseidon::FileCategory;
using Poseidon::FileCategoryName;

TEST_CASE("CategorizeExtension: models", "[Studio][category]")
{
    CHECK(CategorizeExtension(".p3d") == FileCategory::Models);
    CHECK(CategorizeExtension(".P3D") == FileCategory::Models);
    CHECK(CategorizeExtension(".3ds") == FileCategory::Models);
}

TEST_CASE("CategorizeExtension: textures", "[Studio][category]")
{
    CHECK(CategorizeExtension(".paa") == FileCategory::Textures);
    CHECK(CategorizeExtension(".pac") == FileCategory::Textures);
    CHECK(CategorizeExtension(".png") == FileCategory::Textures);
    CHECK(CategorizeExtension(".bmp") == FileCategory::Textures);
    CHECK(CategorizeExtension(".tga") == FileCategory::Textures);
    CHECK(CategorizeExtension(".jpg") == FileCategory::Textures);
    CHECK(CategorizeExtension(".PAA") == FileCategory::Textures);
}

TEST_CASE("CategorizeExtension: sounds", "[Studio][category]")
{
    CHECK(CategorizeExtension(".wav") == FileCategory::Sounds);
    CHECK(CategorizeExtension(".wss") == FileCategory::Sounds);
    CHECK(CategorizeExtension(".ogg") == FileCategory::Sounds);
    CHECK(CategorizeExtension(".lip") == FileCategory::Sounds);
}

TEST_CASE("CategorizeExtension: animations", "[Studio][category]")
{
    CHECK(CategorizeExtension(".rtm") == FileCategory::Animations);
    CHECK(CategorizeExtension(".xob") == FileCategory::Animations);
}

TEST_CASE("CategorizeExtension: scripts", "[Studio][category]")
{
    CHECK(CategorizeExtension(".sqs") == FileCategory::Scripts);
    CHECK(CategorizeExtension(".sqf") == FileCategory::Scripts);
    CHECK(CategorizeExtension(".fsm") == FileCategory::Scripts);
}

TEST_CASE("CategorizeExtension: configs", "[Studio][category]")
{
    CHECK(CategorizeExtension(".cpp") == FileCategory::Configs);
    CHECK(CategorizeExtension(".ext") == FileCategory::Configs);
    CHECK(CategorizeExtension(".cfg") == FileCategory::Configs);
    CHECK(CategorizeExtension(".bin") == FileCategory::Configs);
}

TEST_CASE("CategorizeExtension: archives", "[Studio][category]")
{
    CHECK(CategorizeExtension(".pbo") == FileCategory::Archives);
    CHECK(CategorizeExtension(".PBO") == FileCategory::Archives);
}

TEST_CASE("CategorizeExtension: terrains", "[Studio][category]")
{
    CHECK(CategorizeExtension(".wrp") == FileCategory::Terrains);
}

TEST_CASE("CategorizeExtension: fonts", "[Studio][category]")
{
    CHECK(CategorizeExtension(".fxy") == FileCategory::Fonts);
}

TEST_CASE("CategorizeExtension: other/unknown", "[Studio][category]")
{
    CHECK(CategorizeExtension(".txt") == FileCategory::Other);
    CHECK(CategorizeExtension(".xml") == FileCategory::Other);
    CHECK(CategorizeExtension("") == FileCategory::Other);
    CHECK(CategorizeExtension(".unknown") == FileCategory::Other);
}

TEST_CASE("FileCategoryName returns valid strings", "[Studio][category]")
{
    CHECK(std::string(FileCategoryName(FileCategory::All)) == "All");
    CHECK(std::string(FileCategoryName(FileCategory::Models)) == "Models");
    CHECK(std::string(FileCategoryName(FileCategory::Textures)) == "Textures");
    CHECK(std::string(FileCategoryName(FileCategory::Other)) == "Other");
}
