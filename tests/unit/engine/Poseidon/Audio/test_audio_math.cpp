// Tests for Shared/AudioMath.hpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Audio/Shared/AudioMath.hpp>
#include <cmath>

using Catch::Approx;

TEST_CASE("AudioMath::ReferenceDistance", "[Audio]")
{
    // sqrt(1.0 * 1.0) * 30 = 30
    CHECK(AudioMath::ReferenceDistance(1.f, 1.f) == Approx(30.f));
    // sqrt(0.25 * 1.0) * 30 = 15
    CHECK(AudioMath::ReferenceDistance(0.25f, 1.f) == Approx(15.f));
    // sqrt(1.0 * 4.0) * 30 = 60
    CHECK(AudioMath::ReferenceDistance(1.f, 4.f) == Approx(60.f));
    // Zero volume floors to 1e-6
    CHECK(AudioMath::ReferenceDistance(0.f, 1.f) == Approx(1e-6f));
}

TEST_CASE("AudioMath::MaxDistance", "[Audio]")
{
    CHECK(AudioMath::MaxDistance(1.f, 1.f) == Approx(3000.f));
    CHECK(AudioMath::MaxDistance(0.25f, 1.f) == Approx(1500.f));
}

TEST_CASE("AudioMath::Gain2D", "[Audio]")
{
    // Full pipeline: vol * accom * adjust
    CHECK(AudioMath::Gain2D(0.5f, 2.f, true, 0.8f) == Approx(0.8f));
    // Accommodation disabled: vol * adjust
    CHECK(AudioMath::Gain2D(0.5f, 2.f, false, 0.8f) == Approx(0.4f));
    // Negative result clamped to 0
    CHECK(AudioMath::Gain2D(-1.f, 1.f, true, 1.f) == 0.f);
}

TEST_CASE("AudioMath::Gain3D", "[Audio]")
{
    CHECK(AudioMath::Gain3D(0.75f) == Approx(0.75f));
    CHECK(AudioMath::Gain3D(-0.1f) == 0.f);
}
