#pragma once

#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>

namespace Poseidon
{

// Global opt-in for client-side smoothing of remote (server/AI-controlled)
// units. When false the engine snaps remote units to each network update, the
// faithful Operation Flashpoint behaviour.
extern bool GMPRemoteInterp;

// Tunables for the remote-unit position smoother. Defaults reproduce the
// anti-warp behaviour of later RV engines: ease small/medium error over a few
// frames, hard-cut large error (where easing would read as an obvious slide).
struct RemoteInterpParams
{
    float easeSpeedAbs = 3.0f;     // m/s of positional correction applied even at rest
    float easeSpeedRel = 0.33f;    // extra correction speed per m/s of object speed
    float snapDistAbs = 10.0f;     // error past (abs + rel*speed) is cut, not eased ...
    float snapDistRel = 0.5f;      // ... so faster units tolerate a larger pre-snap error
    float maxSpeedChange = 19.62f; // m/s^2 cap on how fast eased velocity tracks the target (2g)
    float maxAngVel = 2.0f;        // rad/s cap on orientation easing
    float convergePos = 0.01f;     // m: residual below this ends the positional correction
};

struct RemoteInterpResult
{
    Vector3 position;
    Vector3 speed;
    Matrix3 orientation;
    bool active = false;  // a correction was produced; the caller should apply it
    bool snapped = false; // the error exceeded the snap threshold and was cut, not eased
};

// Per-entity smoother that eases a remote unit toward the authoritative state
// from the server instead of snapping on every update. Pure math: no engine,
// network or render dependencies, so it is exercised directly by unit tests.
class RemoteInterp
{
  public:
    RemoteInterp() : _hasTarget(false), _targetPos(VZero), _targetSpeed(VZero), _targetOrient(M3IdentityP) {}

    // Record the latest authoritative state received from the server.
    void SetTarget(Vector3Par pos, Vector3Par speed, const Matrix3& orient);

    // Drop any pending correction (forced snap, ownership change, prediction freeze).
    void Clear() { _hasTarget = false; }

    bool HasTarget() const { return _hasTarget; }
    Vector3Val TargetPosition() const { return _targetPos; }
    Vector3Val TargetSpeed() const { return _targetSpeed; }

    // Advance one simulation step. `cur*` is the entity's post-physics state for
    // this frame; the returned state is where the entity should be placed. The
    // stored target is dead-reckoned forward by `deltaT` so the eased entity
    // chases a moving point rather than the stale position from the last packet.
    RemoteInterpResult Step(float deltaT, Vector3Par curPos, Vector3Par curSpeed, const Matrix3& curOrient,
                            const RemoteInterpParams& p);

  private:
    bool _hasTarget;
    Vector3 _targetPos;
    Vector3 _targetSpeed;
    Matrix3 _targetOrient;
};

} // namespace Poseidon
