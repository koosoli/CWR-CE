// OFP-era scripts commonly use lower-case event names. The event resolver must
// treat them the same as canonical "Dammaged" / "Hit"; otherwise the handlers
// are not registered and this DoDammage probe leaves the player wounded.

triSimFrames 60
triDamagePlayer 0.0
player removeAllEventHandlers "dammaged"
player removeAllEventHandlers "hit"

player addEventHandler ["dammaged", {(_this select 0) setDamage 0}]
player addEventHandler ["hit", {(_this select 0) setDamage 0}]

triDoDammageToPlayer 1.0
triSimFrames 2
triAssertLt [(triPlayerDammage), 0.1]

triEndTest
