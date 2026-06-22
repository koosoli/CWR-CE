// ConfigSystem — records which engine config files actually loaded.
// Apps that ship config.bin + resource.cpp + remaster.cpp (Game) see
// every flag true; apps that ship none (Tetris) see every flag false.

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Core/Config/ConfigSystem.hpp>

TEST_CASE("ConfigSystem starts with every flag false", "[config][system]")
{
    Poseidon::ConfigSystem::Instance().Shutdown();
    CHECK_FALSE(Poseidon::ConfigSystem::Instance().IsConfigAvailable());
    CHECK_FALSE(Poseidon::ConfigSystem::Instance().IsResourceAvailable());
    CHECK_FALSE(Poseidon::ConfigSystem::Instance().IsRemasterAvailable());
}

TEST_CASE("ConfigSystem records load results independently", "[config][system]")
{
    Poseidon::ConfigSystem::Instance().Shutdown();

    Poseidon::ConfigSystem::Instance().MarkConfigLoaded(true);
    CHECK(Poseidon::ConfigSystem::Instance().IsConfigAvailable());
    CHECK_FALSE(Poseidon::ConfigSystem::Instance().IsResourceAvailable());
    CHECK_FALSE(Poseidon::ConfigSystem::Instance().IsRemasterAvailable());

    Poseidon::ConfigSystem::Instance().MarkResourceLoaded(true);
    CHECK(Poseidon::ConfigSystem::Instance().IsConfigAvailable());
    CHECK(Poseidon::ConfigSystem::Instance().IsResourceAvailable());
    CHECK_FALSE(Poseidon::ConfigSystem::Instance().IsRemasterAvailable());

    Poseidon::ConfigSystem::Instance().MarkRemasterLoaded(true);
    CHECK(Poseidon::ConfigSystem::Instance().IsConfigAvailable());
    CHECK(Poseidon::ConfigSystem::Instance().IsResourceAvailable());
    CHECK(Poseidon::ConfigSystem::Instance().IsRemasterAvailable());
}

TEST_CASE("ConfigSystem::Shutdown clears all flags", "[config][system]")
{
    Poseidon::ConfigSystem::Instance().MarkConfigLoaded(true);
    Poseidon::ConfigSystem::Instance().MarkResourceLoaded(true);
    Poseidon::ConfigSystem::Instance().MarkRemasterLoaded(true);

    Poseidon::ConfigSystem::Instance().Shutdown();
    CHECK_FALSE(Poseidon::ConfigSystem::Instance().IsConfigAvailable());
    CHECK_FALSE(Poseidon::ConfigSystem::Instance().IsResourceAvailable());
    CHECK_FALSE(Poseidon::ConfigSystem::Instance().IsRemasterAvailable());

    Poseidon::ConfigSystem::Instance().Shutdown(); // idempotent
    CHECK_FALSE(Poseidon::ConfigSystem::Instance().IsConfigAvailable());
}

TEST_CASE("ConfigSystem::Mark* accepts false to revert state", "[config][system]")
{
    Poseidon::ConfigSystem::Instance().Shutdown();
    Poseidon::ConfigSystem::Instance().MarkConfigLoaded(true);
    CHECK(Poseidon::ConfigSystem::Instance().IsConfigAvailable());

    Poseidon::ConfigSystem::Instance().MarkConfigLoaded(false);
    CHECK_FALSE(Poseidon::ConfigSystem::Instance().IsConfigAvailable());
}
