// Regression fixture for the config-merge array-abort bug (ParamClass::Update).
//
// This mod's bin/config (the deferred mod config, merged into the game config via
// Pars.Update) declares a CfgVehicles whose FIRST entry is vehicleClass[] — an
// array that ALSO exists in the base fixture's CfgVehicles, which is add-only
// (PAReadAndCreate) when the deferred mod config merges over it. It is then
// followed by a NEW unit class.
//
// Pre-fix, the locked-array branch of Update did `return`, abandoning the rest of
// the merge — so ConfigMergeFixUnit (and, in synthetic compatibility fixture's case, all 1097 of its vehicle
// classes) was dropped and never reached the editor. The fix skips the locked
// property and keeps merging. Loaded by config_merge_mod.test.sqf, which asserts
// the class survived via triAssertConfigClass.
class CfgPatches
{
    class ConfigMergeFix
    {
        // Intentionally does NOT list ConfigMergeFixUnit: the engine registers
        // units named in CfgPatches.units[] independently of the CfgVehicles merge,
        // which would mask this bug. synthetic compatibility fixture's bin/config had no CfgPatches at all — its
        // units existed ONLY in CfgVehicles, so the aborted merge wiped them out.
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
    };
};
class CfgVehicles
{
    vehicleClass[] = {};         // leading array — conflicts the base's locked vehicleClass[]
    class ConfigMergeFixUnit {}; // must survive the merge (dropped pre-fix)
};
