#pragma once

#include <cstdint>

namespace Poseidon
{

// Detects timing-based input patterns: double-tap, hold, tap-then-hold.
// Call Update() each frame. Feed edges with OnPress()/OnRelease().

enum class ComboState : uint8_t
{
    Idle,          // waiting for first press
    WaitSecondTap, // first tap done, waiting for second within window
    Holding,       // key is held
    Detected,      // combo detected this frame (auto-resets next Update)
};

class ComboDetector
{
  public:
    float doubleTapWindowMs = 300.0f; // max time between taps for double-tap
    float holdThresholdMs = 400.0f;   // min hold duration to trigger hold

    ComboDetector() = default;

    void Update(float deltaMs);
    void OnPress();
    void OnRelease();
    void Reset();

    bool IsDoubleTap() const { return doubleTapDetected_; }
    bool IsHold() const { return holdDetected_; }
    float HoldDurationMs() const { return holdTimer_; }
    ComboState GetState() const { return state_; }

  private:
    ComboState state_ = ComboState::Idle;
    float timer_ = 0.0f;
    float holdTimer_ = 0.0f;
    bool pressed_ = false;
    bool doubleTapDetected_ = false;
    bool holdDetected_ = false;
};
} // namespace Poseidon

