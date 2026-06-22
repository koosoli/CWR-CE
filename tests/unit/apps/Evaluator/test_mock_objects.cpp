#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "eval_fixture.hpp"
#include <Evaluator/MockObjects.hpp>
#include <string.h>
#include <string>
#include <vector>
#include <Poseidon/Foundation/Strings/RString.hpp>

TEST_CASE("MockObjects - ObjectRegistry create/get/delete", "[evaluator][mock]")
{
    ObjectRegistry reg;

    int id1 = reg.createObject("SyntheticSoldierWest", 100, 0, 200);
    int id2 = reg.createObject("Truck5T", 50, 0, 50);

    SECTION("IDs are unique")
    {
        REQUIRE(id1 != id2);
    }
    SECTION("get returns correct object")
    {
        MockObject* obj = reg.getObject(id1);
        REQUIRE(obj != nullptr);
        REQUIRE(obj->type == "SyntheticSoldierWest");
        REQUIRE(obj->pos[0] == Catch::Approx(100.0f));
        REQUIRE(obj->pos[2] == Catch::Approx(200.0f));
        REQUIRE(obj->alive == true);
        REQUIRE(obj->damage == Catch::Approx(0.0f));
    }
    SECTION("delete removes object")
    {
        reg.deleteObject(id1);
        REQUIRE(reg.getObject(id1) == nullptr);
        REQUIRE(reg.getObject(id2) != nullptr);
    }
    SECTION("get nonexistent returns null")
    {
        REQUIRE(reg.getObject(9999) == nullptr);
    }
}

TEST_CASE("MockObjects - GroupRegistry create/get", "[evaluator][mock]")
{
    GroupRegistry reg;

    int g1 = reg.createGroup(SideWest);
    int g2 = reg.createGroup(SideEast);

    SECTION("IDs are unique")
    {
        REQUIRE(g1 != g2);
    }
    SECTION("get returns correct group")
    {
        MockGroup* grp = reg.getGroup(g1);
        REQUIRE(grp != nullptr);
        REQUIRE(grp->side == SideWest);
        REQUIRE(grp->unitIds.empty());
    }
}

TEST_CASE("MockObjects - GameData types", "[evaluator][mock]")
{
    SECTION("MockGameDataObject")
    {
        GameValue val = makeObjectValue(42);
        REQUIRE(val.GetData()->GetType() == MockGameObject);
        REQUIRE(getObjectId(val) == 42);
        REQUIRE(strcmp(val.GetData()->GetTypeName(), "object") == 0);
    }
    SECTION("MockGameDataGroup")
    {
        GameValue val = makeGroupValue(7);
        REQUIRE(val.GetData()->GetType() == MockGameGroup);
        REQUIRE(getGroupId(val) == 7);
        REQUIRE(strcmp(val.GetData()->GetTypeName(), "group") == 0);
    }
    SECTION("MockGameDataSide")
    {
        GameValue val = makeSideValue(SideWest);
        REQUIRE(val.GetData()->GetType() == MockGameSide);
        REQUIRE(getMockSide(val) == SideWest);
    }
    SECTION("null object id is 0")
    {
        GameValue val = makeObjectValue(0);
        REQUIRE(getObjectId(val) == 0);
    }
    SECTION("non-null object id is not 0")
    {
        GameValue val = makeObjectValue(1);
        REQUIRE(getObjectId(val) != 0);
    }
    SECTION("object equality")
    {
        GameValue a = makeObjectValue(5);
        GameValue b = makeObjectValue(5);
        GameValue c = makeObjectValue(6);
        REQUIRE(a.GetData()->IsEqualTo(b.GetData()) == true);
        REQUIRE(a.GetData()->IsEqualTo(c.GetData()) == false);
    }
    SECTION("side text")
    {
        REQUIRE(strcmp(makeSideValue(SideWest).GetData()->GetText(), "WEST") == 0);
        REQUIRE(strcmp(makeSideValue(SideEast).GetData()->GetText(), "EAST") == 0);
        REQUIRE(strcmp(makeSideValue(SideResistance).GetData()->GetText(), "GUER") == 0);
        REQUIRE(strcmp(makeSideValue(SideCivilian).GetData()->GetText(), "CIV") == 0);
    }
}

TEST_CASE("Object commands - alive/typeOf/name", "[evaluator][mock][commands]")
{
    EvalFixture f;

    // Create a unit via the registry directly
    int id = f.state().objects.createObject("SyntheticSoldierWest", 100, 0, 200);
    f.state().Execute(("_u = objNull"));

    // Store the object in a variable via direct assignment
    f.state().VarSet("testObj", makeObjectValue(id));

    SECTION("alive returns true for new object")
    {
        GameValue r = f.state().Evaluate("alive testObj");
        REQUIRE((bool)r == true);
    }
    SECTION("typeOf returns object type")
    {
        GameValue r = f.state().Evaluate("typeOf testObj");
        REQUIRE(strcmp(((GameStringType)r).Data(), "SyntheticSoldierWest") == 0);
    }
    SECTION("name returns object name")
    {
        GameValue r = f.state().Evaluate("name testObj");
        REQUIRE(strcmp(((GameStringType)r).Data(), "SyntheticSoldierWest") == 0);
    }
}

TEST_CASE("Object commands - getPos/setPos", "[evaluator][mock][commands]")
{
    EvalFixture f;
    int id = f.state().objects.createObject("SyntheticSoldierWest", 100, 0, 200);
    f.state().VarSet("testObj", makeObjectValue(id));

    SECTION("getPos returns position array")
    {
        GameValue r = f.state().Evaluate("getPos testObj");
        REQUIRE(r.GetType() == GameArray);
        const GameArrayType& arr = r;
        REQUIRE(arr.Size() == 3);
        REQUIRE((float)arr[0] == Catch::Approx(100.0f));
        REQUIRE((float)arr[1] == Catch::Approx(0.0f));
        REQUIRE((float)arr[2] == Catch::Approx(200.0f));
    }
    SECTION("setPos updates position")
    {
        f.state().Execute("testObj setPos [50, 10, 75]");
        GameValue r = f.state().Evaluate("getPos testObj");
        const GameArrayType& arr = r;
        REQUIRE((float)arr[0] == Catch::Approx(50.0f));
        REQUIRE((float)arr[1] == Catch::Approx(10.0f));
        REQUIRE((float)arr[2] == Catch::Approx(75.0f));
    }
}

TEST_CASE("Object commands - setDammage makes alive false", "[evaluator][mock][commands]")
{
    EvalFixture f;
    int id = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    f.state().VarSet("testObj", makeObjectValue(id));

    f.state().Execute("testObj setDammage 1.0");
    GameValue alive = f.state().Evaluate("alive testObj");
    REQUIRE((bool)alive == false);

    GameValue dmg = f.state().Evaluate("getDammage testObj");
    REQUIRE((float)dmg == Catch::Approx(1.0f));
}

TEST_CASE("Object commands - setDir/getDir", "[evaluator][mock][commands]")
{
    EvalFixture f;
    int id = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    f.state().VarSet("testObj", makeObjectValue(id));

    f.state().Execute("testObj setDir 90");
    GameValue r = f.state().Evaluate("getDir testObj");
    REQUIRE((float)r == Catch::Approx(90.0f));
}

TEST_CASE("Object commands - isNull", "[evaluator][mock][commands]")
{
    EvalFixture f;

    SECTION("objNull is null")
    {
        GameValue r = f.state().Evaluate("isNull objNull");
        REQUIRE((bool)r == true);
    }
    SECTION("real object is not null")
    {
        int id = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
        f.state().VarSet("testObj", makeObjectValue(id));
        GameValue r = f.state().Evaluate("isNull testObj");
        REQUIRE((bool)r == false);
    }
}

TEST_CASE("Object commands - player nular", "[evaluator][mock][commands]")
{
    EvalFixture f;
    GameValue p = f.state().Evaluate("player");
    REQUIRE(p.GetData() != nullptr);
    REQUIRE(p.GetData()->GetType() == MockGameObject);
    REQUIRE(getObjectId(p) != 0);

    // player should be alive
    GameValue alive = f.state().Evaluate("alive player");
    REQUIRE((bool)alive == true);
}

TEST_CASE("Object commands - setDamage alias", "[evaluator][mock][commands]")
{
    EvalFixture f;
    int id = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    f.state().VarSet("testObj", makeObjectValue(id));

    f.state().Execute("testObj setDamage 0.5");
    GameValue dmg = f.state().Evaluate("getDammage testObj");
    REQUIRE((float)dmg == Catch::Approx(0.5f));
}

TEST_CASE("Object commands - allowDamage alias (one M) behaves identically to allowDammage",
          "[evaluator][mock][commands]")
{
    // The grammatically-correct one-M spelling must work just like the
    // Synthetic-era two-M allowDammage. Both resolve to the same handler.
    EvalFixture f;
    int id = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    f.state().VarSet("testObj", makeObjectValue(id));

    f.state().Execute("testObj allowDamage false");
    f.state().Execute("testObj setDamage 1.0");

    GameValue alive = f.state().Evaluate("alive testObj");
    REQUIRE((bool)alive == true); // invulnerable — the lethal setDamage was blocked

    GameValue dmg = f.state().Evaluate("getDammage testObj");
    REQUIRE((float)dmg == Catch::Approx(0.0f));

    // Re-enabling damage with the one-M spelling lets it through again.
    f.state().Execute("testObj allowDamage true");
    f.state().Execute("testObj setDamage 1.0");
    GameValue aliveAfter = f.state().Evaluate("alive testObj");
    REQUIRE((bool)aliveAfter == false);
}

TEST_CASE("Object commands - allowDammage blocks scripted setDammage increases but still allows repair",
          "[evaluator][mock][commands]")
{
    EvalFixture f;
    int id = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    f.state().VarSet("testObj", makeObjectValue(id));

    SECTION("blocked while invulnerable")
    {
        f.state().Execute("testObj allowDammage false");
        f.state().Execute("testObj setDammage 1.0");

        GameValue alive = f.state().Evaluate("alive testObj");
        REQUIRE((bool)alive == true);

        GameValue dmg = f.state().Evaluate("getDammage testObj");
        REQUIRE((float)dmg == Catch::Approx(0.0f));
    }

    SECTION("repair still works while invulnerable")
    {
        f.state().Execute("testObj setDammage 0.75");
        f.state().Execute("testObj allowDammage false");
        f.state().Execute("testObj setDammage 0.25");

        GameValue alive = f.state().Evaluate("alive testObj");
        REQUIRE((bool)alive == true);

        GameValue dmg = f.state().Evaluate("getDammage testObj");
        REQUIRE((float)dmg == Catch::Approx(0.25f));
    }
}

// --- Creation commands tests ---

TEST_CASE("Creation - createGroup and createUnit workflow", "[evaluator][mock][creation]")
{
    EvalFixture f;

    f.state().Execute("cgrp = createGroup west");
    GameValue grp = f.state().Evaluate("cgrp");
    REQUIRE(grp.GetData() != nullptr);
    REQUIRE(grp.GetData()->GetType() == MockGameGroup);

    f.state().Execute("cu1 = createUnit [\"SyntheticSoldierWest\", [100,0,200], cgrp]");
    f.state().Execute("cu2 = createUnit [\"SyntheticOfficerWest\", [110,0,200], cgrp]");

    GameValue units = f.state().Evaluate("units cgrp");
    REQUIRE(units.GetType() == GameArray);
    const GameArrayType& arr = units;
    REQUIRE(arr.Size() == 2);

    GameValue ldr = f.state().Evaluate("leader cgrp");
    REQUIRE(ldr.GetData()->GetType() == MockGameObject);
    GameValue u1 = f.state().Evaluate("cu1");
    REQUIRE(getObjectId(ldr) == getObjectId(u1));
}

TEST_CASE("Creation - join moves between groups", "[evaluator][mock][creation]")
{
    EvalFixture f;

    f.state().Execute("jg1 = createGroup west");
    f.state().Execute("jg2 = createGroup east");
    f.state().Execute("ju = createUnit [\"SyntheticSoldierWest\", [0,0,0], jg1]");

    GameValue u1 = f.state().Evaluate("units jg1");
    REQUIRE(((const GameArrayType&)u1).Size() == 1);

    f.state().Execute("ju join jg2");

    GameValue u1After = f.state().Evaluate("units jg1");
    REQUIRE(((const GameArrayType&)u1After).Size() == 0);
    GameValue u2After = f.state().Evaluate("units jg2");
    REQUIRE(((const GameArrayType&)u2After).Size() == 1);

    GameValue objGrp = f.state().Evaluate("group ju");
    GameValue g2 = f.state().Evaluate("jg2");
    REQUIRE(getGroupId(objGrp) == getGroupId(g2));
}

TEST_CASE("Creation - createVehicle", "[evaluator][mock][creation]")
{
    EvalFixture f;

    f.state().Execute("cv = createVehicle [\"Truck5T\", [50,0,100]]");
    GameValue v = f.state().Evaluate("cv");
    REQUIRE(v.GetData() != nullptr);
    REQUIRE(v.GetData()->GetType() == MockGameObject);

    GameValue t = f.state().Evaluate("typeOf cv");
    REQUIRE(strcmp(((GameStringType)t).Data(), "Truck5T") == 0);

    GameValue pos = f.state().Evaluate("getPos cv");
    const GameArrayType& arr = pos;
    REQUIRE((float)arr[0] == Catch::Approx(50.0f));
    REQUIRE((float)arr[2] == Catch::Approx(100.0f));
}

TEST_CASE("Creation - side constants", "[evaluator][mock][creation]")
{
    EvalFixture f;

    SECTION("west")
    {
        GameValue r = f.state().Evaluate("west");
        REQUIRE(getMockSide(r) == SideWest);
    }
    SECTION("east")
    {
        GameValue r = f.state().Evaluate("east");
        REQUIRE(getMockSide(r) == SideEast);
    }
    SECTION("resistance")
    {
        GameValue r = f.state().Evaluate("resistance");
        REQUIRE(getMockSide(r) == SideResistance);
    }
    SECTION("civilian")
    {
        GameValue r = f.state().Evaluate("civilian");
        REQUIRE(getMockSide(r) == SideCivilian);
    }
}

TEST_CASE("Creation - deleteVehicle", "[evaluator][mock][creation]")
{
    EvalFixture f;

    f.state().Execute("dv = createVehicle [\"Truck5T\", [0,0,0]]");
    GameValue v = f.state().Evaluate("dv");
    int id = getObjectId(v);
    REQUIRE(f.state().objects.getObject(id) != nullptr);

    f.state().Execute("deleteVehicle dv");
    REQUIRE(f.state().objects.getObject(id) == nullptr);
}

// --- Property commands tests ---

TEST_CASE("Property - distance calculation", "[evaluator][mock][property]")
{
    EvalFixture f;
    int id1 = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    int id2 = f.state().objects.createObject("SyntheticSoldierWest", 3, 0, 4);
    f.state().VarSet("pObj1", makeObjectValue(id1));
    f.state().VarSet("pObj2", makeObjectValue(id2));

    GameValue r = f.state().Evaluate("pObj1 distance pObj2");
    REQUIRE((float)r == Catch::Approx(5.0f));
}

TEST_CASE("Property - vehicle returns self", "[evaluator][mock][property]")
{
    EvalFixture f;
    int id = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    f.state().VarSet("pvObj", makeObjectValue(id));

    GameValue r = f.state().Evaluate("vehicle pvObj");
    REQUIRE(getObjectId(r) == id);
}

TEST_CASE("Property - side returns group side", "[evaluator][mock][property]")
{
    EvalFixture f;
    f.state().Execute("psg = createGroup west");
    f.state().Execute("psu = createUnit [\"SyntheticSoldierWest\", [0,0,0], psg]");

    GameValue r = f.state().Evaluate("side psu");
    REQUIRE(getMockSide(r) == SideWest);
}

TEST_CASE("Property - damage alias", "[evaluator][mock][property]")
{
    EvalFixture f;
    int id = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    f.state().VarSet("pdObj", makeObjectValue(id));
    f.state().Execute("pdObj setDammage 0.75");

    GameValue r = f.state().Evaluate("damage pdObj");
    REQUIRE((float)r == Catch::Approx(0.75f));
}

TEST_CASE("Property - fuel/speed/rating/score defaults", "[evaluator][mock][property]")
{
    EvalFixture f;
    int id = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    f.state().VarSet("pfObj", makeObjectValue(id));

    REQUIRE((float)f.state().Evaluate("fuel pfObj") == Catch::Approx(1.0f));
    REQUIRE((float)f.state().Evaluate("speed pfObj") == Catch::Approx(0.0f));
    REQUIRE((float)f.state().Evaluate("rating pfObj") == Catch::Approx(0.0f));
    REQUIRE((float)f.state().Evaluate("score pfObj") == Catch::Approx(0.0f));
}

TEST_CASE("Property - in operator with objects", "[evaluator][mock][property]")
{
    EvalFixture f;
    int id1 = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    int id2 = f.state().objects.createObject("SyntheticSoldierWest", 10, 0, 0);
    int id3 = f.state().objects.createObject("SyntheticSoldierWest", 20, 0, 0);
    f.state().VarSet("inObj1", makeObjectValue(id1));
    f.state().VarSet("inObj2", makeObjectValue(id2));
    f.state().VarSet("inObj3", makeObjectValue(id3));

    f.state().Execute("inArr = [inObj1, inObj2]");

    SECTION("object in array returns true")
    {
        GameValue r = f.state().Evaluate("inObj1 in inArr");
        REQUIRE((bool)r == true);
    }
    SECTION("object not in array returns false")
    {
        GameValue r = f.state().Evaluate("inObj3 in inArr");
        REQUIRE((bool)r == false);
    }
}

TEST_CASE("Property - countType", "[evaluator][mock][property]")
{
    EvalFixture f;
    f.state().Execute("ctg = createGroup west");
    f.state().Execute("ct1 = createUnit [\"SyntheticSoldierWest\", [0,0,0], ctg]");
    f.state().Execute("ct2 = createUnit [\"SyntheticSoldierWest\", [10,0,0], ctg]");
    f.state().Execute("ct3 = createUnit [\"SyntheticOfficerWest\", [20,0,0], ctg]");
    f.state().Execute("ctArr = [ct1, ct2, ct3]");

    GameValue r = f.state().Evaluate("countType [\"SyntheticSoldierWest\", ctArr]");
    REQUIRE((float)r == Catch::Approx(2.0f));

    GameValue r2 = f.state().Evaluate("countType [\"SyntheticOfficerWest\", ctArr]");
    REQUIRE((float)r2 == Catch::Approx(1.0f));

    GameValue r3 = f.state().Evaluate("countType [\"Truck5T\", ctArr]");
    REQUIRE((float)r3 == Catch::Approx(0.0f));
}

TEST_CASE("Property - nearestObject", "[evaluator][mock][property]")
{
    EvalFixture f;
    f.state().Execute("no1 = createVehicle [\"Flag\", [10,0,0]]");
    f.state().Execute("no2 = createVehicle [\"Flag\", [100,0,0]]");

    GameValue r = f.state().Evaluate("nearestObject [[0,0,0], \"Flag\"]");
    REQUIRE(r.GetData()->GetType() == MockGameObject);
    GameValue n1 = f.state().Evaluate("no1");
    REQUIRE(getObjectId(r) == getObjectId(n1));
}

// --- Variable & action commands tests ---

TEST_CASE("Variable - setVariable/getVariable round-trip", "[evaluator][mock][variable]")
{
    EvalFixture f;
    int id = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    f.state().VarSet("vObj", makeObjectValue(id));

    f.state().Execute("vObj setVariable [\"myTag\", 42]");
    GameValue r = f.state().Evaluate("vObj getVariable \"myTag\"");
    REQUIRE((float)r == Catch::Approx(42.0f));
}

TEST_CASE("Variable - getVariable missing returns nil", "[evaluator][mock][variable]")
{
    EvalFixture f;
    int id = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    f.state().VarSet("vObj2", makeObjectValue(id));

    GameValue r = f.state().Evaluate("vObj2 getVariable \"nope\"");
    REQUIRE(r.GetNil() == true);
}

TEST_CASE("Variable - setVariable with string value", "[evaluator][mock][variable]")
{
    EvalFixture f;
    int id = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    f.state().VarSet("vObj3", makeObjectValue(id));

    f.state().Execute("vObj3 setVariable [\"name\", \"John\"]");
    GameValue r = f.state().Evaluate("vObj3 getVariable \"name\"");
    REQUIRE(r.GetType() == GameString);
    REQUIRE(strcmp(((GameStringType)r).Data(), "John") == 0);
}

TEST_CASE("Variable - addAction returns incrementing ids", "[evaluator][mock][variable]")
{
    EvalFixture f;
    int id = f.state().objects.createObject("SyntheticSoldierWest", 0, 0, 0);
    f.state().VarSet("aObj", makeObjectValue(id));

    f.state().Execute("act1 = aObj addAction [\"Hello\", \"\"]");
    f.state().Execute("act2 = aObj addAction [\"World\", \"\"]");

    GameValue a1 = f.state().Evaluate("act1");
    GameValue a2 = f.state().Evaluate("act2");
    REQUIRE((float)a1 == Catch::Approx(0.0f));
    REQUIRE((float)a2 == Catch::Approx(1.0f));
}
