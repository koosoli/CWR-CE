// Fixture for card #146 crash #6: a weapon with >10 magazine slots.
// _magazineSlots gets one entry per (muzzle x mode). BadManyMags inherits SyntheticMagazine and
// lists 11 modes, so a soldier holding it has 11 magazine slots. Loaded by
// manymags_crash.test via --mod @manymags.
class CfgPatches
{
    class ManyMagsFixture
    {
        units[] = {};
        weapons[] = {"BadManyMags"};
        requiredVersion = 0.1;
    };
};
class CfgWeapons
{
    class SyntheticMagazine {};                  // external — merges with the base fixture's SyntheticMagazine
    class BadManyMags : SyntheticMagazine
    {
        scopeWeapon = 2;
        displayName = "Bad Many Mags";
        modes[] = {"Single", "Burst", "M03", "M04", "M05", "M06", "M07", "M08", "M09", "M10", "M11"};
        class Single {};           // re-declare (merges with SyntheticMagazine's) so M03.. can inherit it
        class M03 : Single {};
        class M04 : Single {};
        class M05 : Single {};
        class M06 : Single {};
        class M07 : Single {};
        class M08 : Single {};
        class M09 : Single {};
        class M10 : Single {};
        class M11 : Single {};
    };
};
