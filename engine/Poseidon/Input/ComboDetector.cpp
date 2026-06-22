#include <Poseidon/Input/ComboDetector.hpp>

namespace Poseidon
{

void ComboDetector::Update(float deltaMs)
{
    doubleTapDetected_ = false;

    switch (state_)
    {
        case ComboState::WaitSecondTap:
            timer_ += deltaMs;
            if (timer_ > doubleTapWindowMs)
            {
                state_ = ComboState::Idle;
                timer_ = 0.0f;
            }
            break;

        case ComboState::Holding:
            holdTimer_ += deltaMs;
            if (holdTimer_ >= holdThresholdMs && !holdDetected_)
                holdDetected_ = true;
            break;

        default:
            break;
    }
}

void ComboDetector::OnPress()
{
    pressed_ = true;

    if (state_ == ComboState::WaitSecondTap)
    {
        doubleTapDetected_ = true;
        state_ = ComboState::Holding;
        holdTimer_ = 0.0f;
        holdDetected_ = false;
    }
    else
    {
        state_ = ComboState::Holding;
        holdTimer_ = 0.0f;
        holdDetected_ = false;
    }
}

void ComboDetector::OnRelease()
{
    pressed_ = false;

    if (state_ == ComboState::Holding)
    {
        if (holdTimer_ < doubleTapWindowMs)
        {
            state_ = ComboState::WaitSecondTap;
            timer_ = 0.0f;
        }
        else
        {
            state_ = ComboState::Idle;
        }
        holdDetected_ = false;
    }
}

void ComboDetector::Reset()
{
    state_ = ComboState::Idle;
    timer_ = 0.0f;
    holdTimer_ = 0.0f;
    pressed_ = false;
    doubleTapDetected_ = false;
    holdDetected_ = false;
}
} // namespace Poseidon
