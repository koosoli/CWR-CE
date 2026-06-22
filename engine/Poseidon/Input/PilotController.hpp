#pragma once

#include <Poseidon/Input/VehicleControls.hpp>
#include <Poseidon/Input/InputContext.hpp>
#include <Poseidon/Input/ResponseCurve.hpp>

namespace Poseidon
{

// PilotController — abstract interface that produces VehicleControls from input.
// Subclasses read from InputSubsystem and apply context-appropriate response curves.
class PilotController
{
  public:
    virtual ~PilotController() = default;

    // Produce controls for this frame
    virtual VehicleControls Poll(float deltaT) = 0;

    // Which input context this controller serves
    virtual InputContext GetContext() const = 0;

    // Response curve configuration
    void SetSteeringCurve(ResponseCurve curve) { steeringCurve_ = curve; }
    void SetThrottleCurve(ResponseCurve curve) { throttleCurve_ = curve; }
    void SetPitchCurve(ResponseCurve curve) { pitchCurve_ = curve; }
    void SetRollCurve(ResponseCurve curve) { rollCurve_ = curve; }
    void SetYawCurve(ResponseCurve curve) { yawCurve_ = curve; }

    const ResponseCurve& GetSteeringCurve() const { return steeringCurve_; }
    const ResponseCurve& GetThrottleCurve() const { return throttleCurve_; }
    const ResponseCurve& GetPitchCurve() const { return pitchCurve_; }
    const ResponseCurve& GetRollCurve() const { return rollCurve_; }
    const ResponseCurve& GetYawCurve() const { return yawCurve_; }

  protected:
    ResponseCurve steeringCurve_ = ResponseCurve::Stick();
    ResponseCurve throttleCurve_ = ResponseCurve::Trigger();
    ResponseCurve pitchCurve_ = ResponseCurve::Stick();
    ResponseCurve rollCurve_ = ResponseCurve::Stick();
    ResponseCurve yawCurve_ = ResponseCurve::Stick();
};
} // namespace Poseidon

