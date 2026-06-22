#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Poseidon
{

enum class CurveType : uint8_t
{
    Linear,      // output = input (after deadzone)
    Quadratic,   // output = input^2 (signed)
    Cubic,       // output = input^3
    SCurve,      // smoothstep-like: 3x^2 - 2x^3
    Exponential, // output = sign * |input|^exponent
};

// Transforms raw analog input [-1,+1] into shaped output.
// Applied per-axis for sticks/triggers to allow per-vehicle/per-context tuning.
struct ResponseCurve
{
    CurveType type = CurveType::Linear;
    float deadzone = 0.0f;   // inner deadzone radius (0..1)
    float saturation = 1.0f; // outer cutoff (0..1), values beyond are clamped to ±1
    float exponent = 2.0f;   // used only by Exponential type
    float sensitivity = 1.0f; // final multiplier

    // Apply curve to raw input value in [-1, +1], returns shaped output in [-1, +1]
    float Apply(float raw) const
    {
        float v = std::clamp(raw, -1.0f, 1.0f);
        float sign = v >= 0.0f ? 1.0f : -1.0f;
        float abs = v >= 0.0f ? v : -v;

        if (abs < deadzone)
            return 0.0f;

        float range = saturation - deadzone;
        if (range <= 0.0f)
            return 0.0f;
        float normalized = std::clamp((abs - deadzone) / range, 0.0f, 1.0f);

        float shaped = 0.0f;
        switch (type)
        {
            case CurveType::Linear:
                shaped = normalized;
                break;
            case CurveType::Quadratic:
                shaped = normalized * normalized;
                break;
            case CurveType::Cubic:
                shaped = normalized * normalized * normalized;
                break;
            case CurveType::SCurve:
                shaped = normalized * normalized * (3.0f - 2.0f * normalized);
                break;
            case CurveType::Exponential:
                shaped = std::pow(normalized, exponent);
                break;
        }

        return std::clamp(sign * shaped * sensitivity, -1.0f, 1.0f);
    }

    static constexpr ResponseCurve Default() { return {}; }
    static constexpr ResponseCurve Stick(float dz = 0.21f)
    {
        ResponseCurve c;
        c.deadzone = dz;
        return c;
    }
    static constexpr ResponseCurve Trigger(float dz = 0.10f)
    {
        ResponseCurve c;
        c.deadzone = dz;
        c.saturation = 1.0f;
        return c;
    }
    static constexpr ResponseCurve FlightStick()
    {
        ResponseCurve c;
        c.type = CurveType::Quadratic;
        c.deadzone = 0.15f;
        return c;
    }
};
} // namespace Poseidon

