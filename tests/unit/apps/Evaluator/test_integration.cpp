// Integration tests - realistic synthetic mission scripts
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "eval_fixture.hpp"
#include "test_fixtures.hpp"
#include <Evaluator/SqsRunner.hpp>
#include <fstream>
#include <sstream>
#include <catch2/matchers/catch_matchers.hpp>
#include <string>

using Catch::Matchers::ContainsSubstring;

static std::string readFile(const std::string& path)
{
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// --- SQS Integration Tests ---

TEST_CASE("Integration: patrol.sqs moves unit through waypoints", "[integration][sqs]")
{
    EvalFixture f;
    f.state().clearOutput();

    // Set up: create group, unit, waypoints
    f.state().EvaluateMultiple("_grp = createGroup west;"
                               "_unit = createUnit [\"SyntheticSoldierWest\", [0,0,0], _grp];");

    SqsRunner runner("patrol.sqs");
    std::string content = readFile(GET_FIXTURE("patrol.sqs"));
    REQUIRE(!content.empty());
    runner.parse(content);

    // Build argument: [unit, waypoints]
    GameArrayType waypoints;
    GameArrayType wp1, wp2, wp3;
    wp1.Add(GameValue(100.0f));
    wp1.Add(GameValue(0.0f));
    wp1.Add(GameValue(200.0f));
    wp2.Add(GameValue(300.0f));
    wp2.Add(GameValue(0.0f));
    wp2.Add(GameValue(400.0f));
    wp3.Add(GameValue(100.0f));
    wp3.Add(GameValue(0.0f));
    wp3.Add(GameValue(600.0f));

    GameArrayType wpArray;
    wpArray.Add(GameValue(wp1));
    wpArray.Add(GameValue(wp2));
    wpArray.Add(GameValue(wp3));

    // Get the unit object from state
    GameValue unit = f.state().VarGet("_unit");
    GameArrayType args;
    args.Add(unit);
    args.Add(GameValue(wpArray));

    int ret = runner.run(f.state(), GameValue(args));
    REQUIRE(ret == 0);

    std::string out = f.state().getOutput();
    REQUIRE_THAT(out, ContainsSubstring("reached waypoint 0"));
    REQUIRE_THAT(out, ContainsSubstring("reached waypoint 1"));
    REQUIRE_THAT(out, ContainsSubstring("reached waypoint 2"));
    REQUIRE_THAT(out, ContainsSubstring("patrol complete (3 waypoints)"));
}

TEST_CASE("Integration: damage_monitor.sqs tracks destruction", "[integration][sqs]")
{
    EvalFixture f;
    f.state().clearOutput();

    GameValue veh = f.state().EvaluateMultiple("createVehicle [\"Truck\", [50,0,50]]");
    REQUIRE(f.state().GetLastError() == EvalOK);

    SqsRunner runner("damage_monitor.sqs");
    std::string content = readFile(GET_FIXTURE("damage_monitor.sqs"));
    REQUIRE(!content.empty());
    runner.parse(content);

    GameArrayType args;
    args.Add(veh);

    int ret = runner.run(f.state(), GameValue(args));
    REQUIRE(ret == 0);

    std::string out = f.state().getOutput();
    REQUIRE_THAT(out, ContainsSubstring("Monitoring"));
    REQUIRE_THAT(out, ContainsSubstring("Truck"));
    // Should eventually be destroyed (0.3 * 4 = 1.2 >= 1.0)
    REQUIRE_THAT(out, ContainsSubstring("destroyed"));
}

TEST_CASE("Integration: mission_init.sqs full mission setup", "[integration][sqs]")
{
    EvalFixture f;
    f.state().clearOutput();

    SqsRunner runner("mission_init.sqs");
    std::string content = readFile(GET_FIXTURE("mission_init.sqs"));
    REQUIRE(!content.empty());
    runner.parse(content);

    int ret = runner.run(f.state(), GameValue());
    REQUIRE(ret == 0);

    std::string out = f.state().getOutput();
    // West group created with 3 units, officer as leader
    REQUIRE_THAT(out, ContainsSubstring("West group: 3 units"));
    REQUIRE_THAT(out, ContainsSubstring("leader: SyntheticOfficerWest"));
    // East group with 2 units
    REQUIRE_THAT(out, ContainsSubstring("East group: 2 units"));
    // Supply truck variables
    REQUIRE_THAT(out, ContainsSubstring("cargo: ammo"));
    REQUIRE_THAT(out, ContainsSubstring("capacity: 100"));
    // All west alive
    REQUIRE_THAT(out, ContainsSubstring("All west units alive: true"));
    // Enemy killed
    REQUIRE_THAT(out, ContainsSubstring("Enemy1 alive: false"));
    // Enemy count: 1 alive of 2
    REQUIRE_THAT(out, ContainsSubstring("Enemy alive: 1 of 2"));
}

// --- SQF Integration Tests ---

TEST_CASE("Integration: squad_management.sqf full squad lifecycle", "[integration][sqf]")
{
    EvalFixture f;
    f.state().clearOutput();

    std::string content = readFile(GET_FIXTURE("squad_management.sqf"));
    REQUIRE(!content.empty());

    GameVarSpace local(f.state().GetContext());
    f.state().BeginContext(&local);
    f.state().EvaluateMultiple(content.c_str());
    REQUIRE(f.state().GetLastError() == EvalOK);
    f.state().EndContext();

    std::string out = f.state().getOutput();
    REQUIRE_THAT(out, ContainsSubstring("Squad size: 4"));
    REQUIRE_THAT(out, ContainsSubstring("Leader: SyntheticOfficerWest"));
    REQUIRE_THAT(out, ContainsSubstring("Unit 0 role: Commander"));
    REQUIRE_THAT(out, ContainsSubstring("Unit 3 role: MG"));
    // 1 killed (damage 1.0), 1 wounded (0.5), 2 healthy = 3 alive
    REQUIRE_THAT(out, ContainsSubstring("Alive: 3 of 4"));
    // Distance from [0,0,0] to [15,0,0] = 15
    REQUIRE_THAT(out, ContainsSubstring("Squad spread: 15"));
}

TEST_CASE("Integration: utility_functions.sqf call and nested logic", "[integration][sqf]")
{
    EvalFixture f;
    f.state().clearOutput();

    std::string content = readFile(GET_FIXTURE("utility_functions.sqf"));
    REQUIRE(!content.empty());

    GameVarSpace local(f.state().GetContext());
    f.state().BeginContext(&local);
    f.state().EvaluateMultiple(content.c_str());
    REQUIRE(f.state().GetLastError() == EvalOK);
    f.state().EndContext();

    std::string out = f.state().getOutput();
    // 3^2 + 4^2 = 25, 6^2 + 8^2 = 100
    REQUIRE_THAT(out, ContainsSubstring("1-2: 25"));
    REQUIRE_THAT(out, ContainsSubstring("1-3: 100"));
    // Sum
    REQUIRE_THAT(out, ContainsSubstring("Sum of [1..5]: 15"));
    // Max
    REQUIRE_THAT(out, ContainsSubstring("Max of [7,3,9,1,5]: 9"));
    // Names
    REQUIRE_THAT(out, ContainsSubstring("Names: Alpha, Bravo, Charlie"));
}

// --- CLI Integration via exec ---

TEST_CASE("Integration: SQF exec triggers SQS patrol", "[integration][exec]")
{
    EvalFixture f;
    f.state().clearOutput();

    // Create a unit, then exec the patrol script on it
    // The exec command reads the file and runs it synchronously
    GameVarSpace local(f.state().GetContext());
    f.state().BeginContext(&local);

    f.state().EvaluateMultiple("_grp = createGroup west;"
                               "_unit = createUnit [\"SyntheticSoldierWest\", [0,0,0], _grp];");
    REQUIRE(f.state().GetLastError() == EvalOK);

    // Build [unit, waypoints] and exec
    std::string execCmd =
        std::string("_wps = [[50,0,50],[100,0,100]];") + "[_unit, _wps] exec \"" + GET_FIXTURE("patrol.sqs") + "\";";
    f.state().EvaluateMultiple(execCmd.c_str());
    REQUIRE(f.state().GetLastError() == EvalOK);

    f.state().EndContext();

    std::string out = f.state().getOutput();
    REQUIRE_THAT(out, ContainsSubstring("reached waypoint 0"));
    REQUIRE_THAT(out, ContainsSubstring("patrol complete (2 waypoints)"));
}

TEST_CASE("Integration: call with complex argument passing", "[integration][call]")
{
    EvalFixture f;
    f.state().clearOutput();

    GameVarSpace local(f.state().GetContext());
    f.state().BeginContext(&local);

    // Chain of calls: create objects, compute, format
    f.state().EvaluateMultiple("_makeSquad = {"
                               "  private [\"_grp\", \"_count\"];"
                               "  _grp = createGroup (_this select 0);"
                               "  _count = _this select 1;"
                               "  _i = 0;"
                               "  while {_i < _count} do {"
                               "    createUnit [\"SyntheticSoldierWest\", [_i * 10, 0, 0], _grp];"
                               "    _i = _i + 1;"
                               "  };"
                               "  _grp"
                               "};"
                               "_squad = [west, 5] call _makeSquad;"
                               "hint format [\"Created squad with %1 units\", count (units _squad)];");
    REQUIRE(f.state().GetLastError() == EvalOK);

    f.state().EndContext();

    std::string out = f.state().getOutput();
    REQUIRE_THAT(out, ContainsSubstring("Created squad with 5 units"));
}
