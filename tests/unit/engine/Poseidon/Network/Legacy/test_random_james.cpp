#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Network/Legacy/RandomJames.h>

#include <cmath>

TEST_CASE("RandomJames - Construction and initialization", "[network][legacy][randomjames][random]")
{
    SECTION("Default construction")
    {
        RandomJames rng;

        REQUIRE(rng.ok == true);
    }

    SECTION("Construction with seeds")
    {
        RandomJames rng(1234, 5678);

        REQUIRE(rng.ok == true);
    }

    SECTION("Reset with different seeds")
    {
        RandomJames rng;
        rng.reset(9999, 1111);

        REQUIRE(rng.ok == true);
    }
}

TEST_CASE("RandomJames - Uniform random generation", "[network][legacy][randomjames][random]")
{
    RandomJames rng(1802, 9373);

    SECTION("uniformNumber() returns value in [0,1]")
    {
        double val = rng.uniformNumber();

        REQUIRE(val >= 0.0);
        REQUIRE(val <= 1.0);
    }

    SECTION("Multiple uniformNumber() calls give different values")
    {
        double val1 = rng.uniformNumber();
        double val2 = rng.uniformNumber();
        double val3 = rng.uniformNumber();

        REQUIRE(val1 != val2);
        REQUIRE(val2 != val3);
    }

    SECTION("uniformNumbers() fills vector")
    {
        double vec[10];
        rng.uniformNumbers(vec, 10);

        for (int i = 0; i < 10; i++)
        {
            REQUIRE(vec[i] >= 0.0);
            REQUIRE(vec[i] <= 1.0);
        }

        REQUIRE(vec[0] != vec[1]);
        REQUIRE(vec[1] != vec[2]);
    }

    SECTION("Deterministic with same seed")
    {
        RandomJames rng1(1000, 2000);
        RandomJames rng2(1000, 2000);

        for (int i = 0; i < 10; i++)
        {
            double val1 = rng1.uniformNumber();
            double val2 = rng2.uniformNumber();
            REQUIRE(val1 == Catch::Approx(val2));
        }
    }
}

TEST_CASE("RandomJames - Normal distribution", "[network][legacy][randomjames][random]")
{
    RandomJames rng(5555, 6666);

    SECTION("normalNumber() with mean=0, variance=1")
    {
        double val = rng.normalNumber(0.0, 1.0, 4);

        REQUIRE(val >= -10.0);
        REQUIRE(val <= 10.0);
    }

    SECTION("normalNumber() with custom mean and variance")
    {
        double mean = 10.0;
        double variance = 4.0;
        double sum = 0.0;
        int count = 100;

        for (int i = 0; i < count; i++)
        {
            double val = rng.normalNumber(mean, variance, 4);
            sum += val;
        }

        double avg = sum / count;
        double stdDev = std::sqrt(variance);
        REQUIRE(avg == Catch::Approx(mean).margin(stdDev));
    }

    SECTION("normalNumbers() fills vector")
    {
        double vec[10];
        rng.normalNumbers(0.0, 1.0, 4, vec, 10);

        for (int i = 0; i < 10; i++)
        {
            REQUIRE(vec[i] >= -10.0);
            REQUIRE(vec[i] <= 10.0);
        }
    }
}
