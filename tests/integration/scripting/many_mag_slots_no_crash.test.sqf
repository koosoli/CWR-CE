// Regression guard: a soldier with >10 magazine slots must not crash.
//
// In OFP this overflowed a fixed-size magazine-slot array (the bug "vbs has
// fixed"). The remaster stores EntityAI::_magazineSlots in a growable AutoArray
// (AddWeapon adds one slot per muzzle x mode), so it no longer overflows. The
// @manymags fixture's BadManyMags weapon has 11 modes = 11 slots; equip it with
// 12 magazines and fire. If _magazineSlots ever regresses to a fixed array this
// crashes before triEndTest; today it survives.
triSimUntil { time >= 2 }
removeAllWeapons player
player addWeapon "BadManyMags"
_i = 0
while { _i < 12 } do { player addMagazine "M16"; _i = _i + 1 }
player selectWeapon "BadManyMags"
triWaitFrames 10
player fire "BadManyMags"
triWaitFrames 20
triEndTest
