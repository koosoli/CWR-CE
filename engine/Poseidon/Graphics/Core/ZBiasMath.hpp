#pragma once

// Software z-bias coefficient math.
//
// When a backend reports `CanZBias() == false`, callers apply z-bias in
// software by perturbing the projection matrix: `zMult = 1 - zBias * 1e-7`,
// `zAdd = zBias * -2e-7`.  Pure arithmetic, shared between EngineGL33::GetZCoefs
// and any other backend that opts into the software path.
//
// Capability invariant: when CanZBias returns true the backend must implement
// glPolygonOffset (or equivalent) and callers skip the software path — these
// coefficients are NOT applied.  When CanZBias returns false, callers MUST
// apply them, or terrain occludes the UI.

namespace Poseidon
{
namespace render::zbias
{

struct Coefs
{
    float zAdd;
    float zMult;
};

constexpr Coefs SoftwareCoefs(int bias) noexcept
{
    Coefs c{};
    c.zMult = 1.0f - bias * 1e-7f;
    c.zAdd = bias * -2e-7f;
    return c;
}

} // namespace render::zbias

} // namespace Poseidon
