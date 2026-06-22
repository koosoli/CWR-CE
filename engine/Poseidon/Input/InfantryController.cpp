#include <Poseidon/Input/InfantryController.hpp>
#include <Poseidon/Input/InputSubsystem.hpp>

namespace Poseidon
{

VehicleControls InfantryController::Poll(float deltaT)
{
    auto& input = InputSubsystem::Instance();
    VehicleControls ctrl;

    ctrl.throttle = input.GetAction(UAMoveForward) - input.GetAction(UAMoveBack);
    ctrl.steering = input.GetAction(UATurnRight) - input.GetAction(UATurnLeft);

    ctrl.collective = input.GetAction(UAMoveUp) - input.GetAction(UAMoveDown);

    ctrl.roll = input.GetAction(UAMoveRight) - input.GetAction(UAMoveLeft);

    ctrl.fire = input.GetActionToDo(UAFire, false);
    ctrl.toggleWeapons = input.GetActionToDo(UAToggleWeapons, false);
    ctrl.reloadMagazine = input.GetActionToDo(UAReloadMagazine, false);
    ctrl.optics = input.GetActionToDo(UAOptics, false);

    ctrl.lookX = input.GetMouseDeltaX();
    ctrl.lookY = input.GetMouseDeltaY();
    ctrl.lookAround = input.IsLookAroundEnabled();

    ctrl.turbo = input.GetAction(UATurbo) > 0.0f;
    ctrl.slow = input.GetAction(UASlow) > 0.0f;

    return ctrl;
}
} // namespace Poseidon
