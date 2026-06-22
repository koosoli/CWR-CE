// Editor "default time seconds show 01" regression.
//
// Clock stores time as a single 32-bit float fraction-of-year (_timeInYear, with
// OneDay = 1/365). At a May value (~0.35) the float ULP is ~1s, so deriving the
// time-of-day as fastFmod(_timeInYear, OneDay) loses sub-second precision: the
// editor watch second hand (Watch::Animate) floors that value and lands on :01
// for the ArcadeTemplate default (May 10, 07:30) instead of :00.
//
// The fix is Clock::SetTimeOfDay(hour, minute, second), which stores _timeOfDay as
// a [0,1) day fraction at full precision (07:30 -> exactly 0.3125), called right
// after SetTimeInYear (which still sets the coarse _timeInYear, so date/season/light_disc
// are untouched and the save format is unchanged). Broken-state delta: the
// coarse-only path yields a non-zero second hand (:01 for May 10), the fixed path
// yields exactly 00:00 minute/second.

#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Core/Global.hpp>

#include <cmath>

using namespace Poseidon;

namespace
{
// Exact replica of the watch second-hand extraction (Watch::Animate,
// engine/Poseidon/UI/Map/UIMapDialogs.cpp): minute and second floored out of the
// day fraction the same way the rendered hands are.
struct HMS
{
    int hour;
    int minute;
    int second;
};

HMS ExtractWatchHMS(const Clock& clock)
{
    double timeOfDay = clock.GetTimeOfDay();
    double hourFrac = std::fmod(24.0 * timeOfDay, 1.0);
    double minFrac = std::fmod(60.0 * hourFrac, 1.0);
    HMS hms;
    hms.hour = (int)std::floor(24.0 * timeOfDay);
    hms.minute = (int)std::floor(60.0 * hourFrac);
    hms.second = (int)std::floor(60.0 * minFrac);
    return hms;
}

// May 10 (ArcadeTemplate default month=5, day=10), year 1985 (non-leap):
// Jan 31 + Feb 28 + Mar 31 + Apr 30 = 120, + (10 - 1) = 129.
const int DefaultDayOfYear = 129;
} // namespace

TEST_CASE("Clock::SetTimeOfDay yields exact editor seconds", "[clock][time][editor]")
{
    const int day = DefaultDayOfYear;

    // Pre-fix path: the editor / SetDate set only the coarse year-fraction.
    Clock coarse;
    coarse.SetTimeInYear(day * OneDay + 7 * OneHour + 30 * OneMinute + 0.5 * OneSecond);
    coarse.SetYear(1985);
    HMS broken = ExtractWatchHMS(coarse);

    // Fixed path: SetTimeInYear (coarse date, unchanged) + SetTimeOfDay (precise).
    Clock fixed;
    fixed.SetTimeInYear(day * OneDay + 7 * OneHour + 30 * OneMinute + 0.5 * OneSecond);
    fixed.SetTimeOfDay(7, 30);
    fixed.SetYear(1985);
    HMS good = ExtractWatchHMS(fixed);

    // Fixed state: exact 07:30:00.
    REQUIRE(good.hour == 7);
    REQUIRE(good.minute == 30);
    REQUIRE(good.second == 0);

    // Broken-state delta: the coarse-only derivation floors to the reported :01
    // for the default editor time; the fix drives it to :00.
    CHECK(broken.second == 1);

    // _timeInYear is left untouched by the fix, so the coarse date is identical.
    REQUIRE(coarse.GetTimeInYear() == fixed.GetTimeInYear());
}

TEST_CASE("Clock::SetTimeOfDay round-trips representative times", "[clock][time]")
{
    struct Case
    {
        int hour;
        int minute;
    };
    const Case cases[] = {{0, 0}, {7, 30}, {12, 0}, {13, 45}, {23, 55}};
    for (const Case& c : cases)
    {
        Clock clock;
        clock.SetTimeInYear(129 * OneDay + c.hour * OneHour + c.minute * OneMinute + 0.5 * OneSecond);
        clock.SetTimeOfDay(c.hour, c.minute);
        HMS hms = ExtractWatchHMS(clock);
        CHECK(hms.hour == c.hour);
        CHECK(hms.minute == c.minute);
        CHECK(hms.second == 0);
    }
}
