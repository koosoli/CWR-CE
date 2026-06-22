#include <Poseidon/World/Simulation/RemoteInterp.hpp>

#include <Poseidon/Foundation/Math/MathDefs.hpp>
#include <Poseidon/Foundation/Math/MathOpt.hpp>

namespace Poseidon
{

bool GMPRemoteInterp = true;

void RemoteInterp::SetTarget(Vector3Par pos, Vector3Par speed, const Matrix3& orient)
{
    _targetPos = pos;
    _targetSpeed = speed;
    _targetOrient = orient;
    _hasTarget = true;
}

RemoteInterpResult RemoteInterp::Step(float deltaT, Vector3Par curPos, Vector3Par curSpeed, const Matrix3& curOrient,
                                      const RemoteInterpParams& p)
{
    RemoteInterpResult r;
    r.position = curPos;
    r.speed = curSpeed;
    r.orientation = curOrient;

    if (!_hasTarget || deltaT <= 0)
    {
        return r;
    }

    // Dead-reckon the target forward so the eased entity chases where the server
    // unit is now, not where it was when the packet was sent.
    _targetPos += _targetSpeed * deltaT;

    Vector3 err = _targetPos - curPos;
    float dist = err.Size();
    float objSpeed = curSpeed.Size();

    float snapDist = p.snapDistAbs + p.snapDistRel * objSpeed;
    if (dist > snapDist)
    {
        // Too far to ease without an obvious rubber-band: cut straight to target.
        r.position = _targetPos;
        r.speed = _targetSpeed;
        r.orientation = _targetOrient;
        r.active = true;
        r.snapped = true;
        _hasTarget = false;
        return r;
    }

    bool converged = true;

    // Position: step toward the target, capped per frame.
    if (dist > p.convergePos)
    {
        float maxStep = (p.easeSpeedAbs + p.easeSpeedRel * objSpeed) * deltaT;
        float move = floatMin(maxStep, dist);
        r.position = curPos + err * (move / dist);
        r.active = true;
        if (move < dist)
        {
            converged = false;
        }
    }
    else
    {
        r.position = _targetPos;
    }

    // Velocity: ease toward the target speed, capped.
    Vector3 sDif = _targetSpeed - curSpeed;
    float maxSpeedStep = p.maxSpeedChange * deltaT;
    if (sDif.SquareSize() > Square(maxSpeedStep))
    {
        sDif = sDif.Normalized() * maxSpeedStep;
        converged = false;
    }
    if (sDif.SquareSize() > 0)
    {
        r.speed = curSpeed + sDif;
        r.active = true;
    }

    // Orientation: ease toward the target, capped per frame.
    Matrix3 oChange = _targetOrient - curOrient;
    float oScale2 = oChange.Distance2(M3ZeroP);
    if (oScale2 > Square(1e-3f))
    {
        float maxAngStep = p.maxAngVel * deltaT;
        if (oScale2 > Square(maxAngStep))
        {
            oChange *= InvSqrt(oScale2) * maxAngStep;
            converged = false;
        }
        Matrix3 newOrient = curOrient + oChange;
        newOrient.Orthogonalize();
        r.orientation = newOrient;
        r.active = true;
    }

    if (converged)
    {
        _hasTarget = false;
    }
    return r;
}

} // namespace Poseidon
