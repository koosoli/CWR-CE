#pragma once

#include <Poseidon/Input/InputCode.hpp>

namespace Poseidon
{

enum class ActivationMode
{
    OnPress,   // fires once when key goes down (edge)
    OnHold,    // fires continuously while held (level)
    OnRelease, // fires once when key goes up (edge)
};

// A single binding entry: maps an input code to an activation mode
struct InputBinding
{
    InputCode code;
    InputCode modifier;
    ActivationMode mode = ActivationMode::OnHold;
    float scale = 1.0f;

    constexpr InputBinding() = default;
    constexpr InputBinding(InputCode c, ActivationMode m = ActivationMode::OnHold) : code(c), mode(m) {}
    constexpr InputBinding(InputCode c, InputCode mod, ActivationMode m = ActivationMode::OnHold, float s = 1.0f)
        : code(c), modifier(mod), mode(m), scale(s)
    {
    }

    constexpr bool operator==(const InputBinding& other) const
    {
        return code == other.code && modifier == other.modifier && mode == other.mode && scale == other.scale;
    }
};
} // namespace Poseidon

