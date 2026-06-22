// Regression guard for the config-merge array-abort bug (ParamClass::Update).
//
// @configmergemod's bin/config declares a CfgVehicles whose FIRST entry is
// vehicleClass[] — an array that also exists in the base game's CfgVehicles, which
// is add-only (PAReadAndCreate) when the deferred mod config merges over it — then
// a new class ConfigMergeFixUnit. Pre-fix the locked-array merge `return`ed and
// dropped the class (CSLA's "no units in editor" symptom: all its vehicle classes
// were dropped the same way). ConfigMergeFixUnit is deliberately NOT in
// CfgPatches.units[], so it exists only via the CfgVehicles merge (like CSLA's
// units). This asserts the class survived the merge.
//
// Teeth: revert the fix (continue -> return in ParamFile.cpp Update's array branch)
// and this fails with "FAIL:missing config class 'CfgVehicles/ConfigMergeFixUnit'".

triSetLanguage "English"
triSimUntil { triGameMode == 2 }
triAssertEq [(triDisplay), 0]

triAssertConfigClass "CfgVehicles/ConfigMergeFixUnit"
triEndTest
