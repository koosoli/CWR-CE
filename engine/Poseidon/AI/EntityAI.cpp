#include <Poseidon/AI/VehicleAI.hpp>

// Out-of-line: instantiates LLink<Target>(Target*) which needs the complete Target
// (defined in VehicleAI.hpp), so it can't live inline in EntityAI.hpp.

namespace Poseidon
{
LinkTarget::LinkTarget(Target* tgt) : LLink<Target>(tgt) {}

Target* EntityAI::GetFireTarget() const
{
    return _fire._fireTarget;
}
Target* EntityAI::GetHideTarget() const
{
    return _hideTarget;
}

} // namespace Poseidon
