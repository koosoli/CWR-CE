// Regression: createVehicle of a CfgVehicles class with model="" used to crash.
//
// A crewed/AI simulation (soldier here) constructs an entity class (Man) that
// dereferences the type's shape during construction; an empty model leaves the
// shape null, so the soldier crashed in EntityAIType::InitShape, then (guarded)
// in Man::Man -> RecalcGunTransform -> GetWeaponCenter, then rendering proxies —
// the null shape is assumed engine-wide. Fix: NewVehicle refuses to create a
// shape-requiring vehicle whose model is empty (logs an error, returns null).
//
// Method: the @badmodel fixture mod defines `class BadModelUnit : SoldierWB
// { model=""; scope=2; }`. createVehicle it. Pre-fix the game crashed (exit
// 0xC0000005) before triEndTest; post-fix createVehicle returns objNull and the
// game survives, so reaching triEndTest is the assertion.
triSimUntil { time >= 2 }
_v = "BadModelUnit" createVehicle (getpos player)
triWaitFrames 8
triEndTest
