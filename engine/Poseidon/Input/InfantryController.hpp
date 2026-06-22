#pragma once

#include <Poseidon/Input/PilotController.hpp>

namespace Poseidon
{

// InfantryController — produces controls for on-foot infantry movement.
// Reads movement keys (WASD/arrows) and mouse look from InputSubsystem.
// This is a simple concrete example; vehicle controllers follow the same pattern.
class InfantryController : public PilotController
{
  public:
    VehicleControls Poll(float deltaT) override;
    InputContext GetContext() const override { return InputContext::Infantry; }
};
} // namespace Poseidon

