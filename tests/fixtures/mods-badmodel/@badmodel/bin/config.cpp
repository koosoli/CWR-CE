// Fixture for card #146 crash #3: a CfgVehicles class with an empty model="".
// createVehicle of this class must load an empty shape; an unguarded shape load
// crashes the game. Loaded by create_vehicle_empty_model.test via --mod @badmodel.
class CfgPatches
{
    class BadModelFixture
    {
        units[] = {"BadModelUnit"};
        weapons[] = {};
        requiredVersion = 0.1;
    };
};
class CfgVehicles
{
    class SyntheticSoldierWest {};              // external — merges with the base fixture's West soldier
    class BadModelUnit : SyntheticSoldierWest   // a valid, placeable soldier...
    {
        scope = 2;
        displayName = "Bad Model Unit";
        model = "";                  // ...but with no model: the crash trigger
    };
};
