// GitLab #43: a subgroup whose leader starts as vehicle cargo must still think
// and complete a mission-authored GETOUT waypoint.
//
// Broken state: AISubgroup::SelectLeader rejected the cargo passenger because
// !IsUnit(), nulled the subgroup leader, and the authored GETOUT waypoint never
// ran. The passenger stayed inside cargoTruck until this assertion timed out.

triSetLanguage "English"

if ((vehicle cargoPassenger) != cargoTruck) then { "FAIL:fixture passenger did not start in cargo" } else { "OK" }
triAssertSubgroupLeader "cargoPassenger"
(group cargoPassenger) leaveVehicle cargoTruck
[cargoPassenger] orderGetIn false
triSimUntil { (vehicle cargoPassenger) == cargoPassenger }
if ((vehicle cargoPassenger) == cargoPassenger) then { "OK" } else { "FAIL:cargo passenger stayed in vehicle after GETOUT waypoint" }

triEndTest
