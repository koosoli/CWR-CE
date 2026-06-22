#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Poseidon
{

// A single rumble effect with timing.
struct RumbleEffect
{
    float beginMagnitude = 0.0f;
    float endMagnitude = 0.0f;
    float durationMs = 0.0f;
    float elapsedMs = 0.0f;
    bool active = false;

    void Start(float begin, float end, float durationMillis)
    {
        beginMagnitude = std::fabs(begin);
        endMagnitude = std::fabs(end);
        durationMs = durationMillis;
        elapsedMs = 0.0f;
        active = true;
    }

    void Stop()
    {
        active = false;
        elapsedMs = 0.0f;
    }

    // Returns current magnitude [0, 1], advances timer
    float Evaluate(float deltaMs)
    {
        if (!active || durationMs <= 0.0f)
            return 0.0f;

        elapsedMs += deltaMs;
        if (elapsedMs >= durationMs)
        {
            active = false;
            return 0.0f;
        }

        float t = elapsedMs / durationMs;
        return beginMagnitude + (endMagnitude - beginMagnitude) * t;
    }

    bool IsActive() const { return active; }
};

// Manages layered rumble effects (engine + weapon/event).
class RumbleManager
{
  public:
    RumbleManager() = default;

    // Continuous engine rumble (proportional to RPM/speed)
    void SetEngineRumble(float magnitude)
    {
        engineMagnitude_ = std::clamp(std::fabs(magnitude), 0.0f, 1.0f);
    }

    // One-shot weapon/event rumble with ramp
    void PlayEffect(float beginMag, float endMag, float durationMs)
    {
        effect_.Start(beginMag, endMag, durationMs);
    }

    // Stop all effects
    void StopAll()
    {
        engineMagnitude_ = 0.0f;
        effect_.Stop();
    }

    // Evaluate final motor values for this frame.
    // Returns left (low-freq) and right (high-freq) motor magnitudes [0, 1].
    void Evaluate(float deltaMs, float& leftMotor, float& rightMotor)
    {
        float effectMag = effect_.Evaluate(deltaMs);

        // Left motor = low freq (engine feel), right = high freq (weapon snap)
        leftMotor = std::max(engineMagnitude_ * 0.7f, effectMag * 0.5f);
        rightMotor = std::max(engineMagnitude_ * 0.3f, effectMag * 1.0f);

        ApplyThreshold(leftMotor, 0.10f, 0.20f);
        ApplyThreshold(rightMotor, 0.05f, 0.10f);

        leftMotor = std::clamp(leftMotor, 0.0f, 1.0f);
        rightMotor = std::clamp(rightMotor, 0.0f, 1.0f);
    }

    float GetEngineMagnitude() const { return engineMagnitude_; }
    bool IsEffectActive() const { return effect_.IsActive(); }

  private:
    float engineMagnitude_ = 0.0f;
    RumbleEffect effect_;

    static void ApplyThreshold(float& value, float minThreshold, float floorValue)
    {
        if (value < minThreshold)
            value = 0.0f;
        else if (value < floorValue)
            value = floorValue;
    }
};
} // namespace Poseidon

