#include <Poseidon/Input/ViewerControlsApply.hpp>
#include <math.h>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/MathDefs.hpp>

namespace Poseidon
{

namespace
{
constexpr float kTranslateSpeed = 0.01f; // world-units per pixel of LMB drag
constexpr float kRotateSpeed = 0.005f;   // radians per pixel of RMB drag
constexpr float kZoomPerTick = 0.85f;    // dist multiplier per wheel-tick (<1 = closer)
constexpr float kZoomMinDist = 0.10f;    // metres — clamp so we don't pierce the model
constexpr float kZoomMaxDist = 500.0f;   // metres — clamp so we don't fly to fog
constexpr float kPitchClamp = 1.5707f;   // π/2 minus a hair (gimbal guard)

template <class T>
T clamp_(T v, T lo, T hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}
} // namespace

ViewerStateIO ApplyViewerControls(const ViewerControls& c, const ViewerStateIO& in, float deltaT,
                                  float defaultAnimSpeed)
{
    ViewerStateIO out = in;
    out.didReloadTextures = false;
    out.didExitViewer = false;

    // Translate (LMB drag) — slide along camera-aligned axes.  +X drag
    // moves +Aside, +Y drag moves -Up (screen Y grows down, world up
    // grows up; flip the sign so dragging down moves the object down
    // on screen).
    if (c.translateX != 0.0f || c.translateY != 0.0f)
    {
        out.objectPos += c.translateX * kTranslateSpeed * out.cameraAside;
        out.objectPos += -c.translateY * kTranslateSpeed * out.cameraUp;
    }

    // Rotate (RMB drag) — build head/dive in object-local frame from
    // the current Direction().
    if (c.rotateX != 0.0f || c.rotateY != 0.0f)
    {
        // Project current Direction() to spherical (head, dive).
        Vector3 dir = out.objectOrient.Direction();
        float head = -atan2f(dir.X(), dir.Z()) + c.rotateX * kRotateSpeed;
        float xz = sqrtf(dir.X() * dir.X() + dir.Z() * dir.Z());
        float dive = -atan2f(dir.Y(), xz) - c.rotateY * kRotateSpeed;
        dive = clamp_(dive, -kPitchClamp, +kPitchClamp);
        out.objectOrient = Matrix3(MRotationY, head) * Matrix3(MRotationX, dive);
    }

    // Zoom (wheel) — exponential dolly toward the look-at target
    // (objectPos).  Each tick multiplies camera-to-target distance by
    // kZoomPerTick, so visual zoom rate is constant regardless of how
    // far we already are.
    if (c.zoom != 0.0f)
    {
        Vector3 toCam = out.cameraPos - out.objectPos;
        float curDist = toCam.Size();
        if (curDist > 1e-4f)
        {
            Vector3 toCamDir = toCam * (1.0f / curDist);
            float ratio = powf(kZoomPerTick, c.zoom);
            float newDist = clamp_(curDist * ratio, kZoomMinDist, kZoomMaxDist);
            out.cameraPos = out.objectPos + toCamDir * newDist;
        }
    }

    // Animation transport.
    if (c.animScrub != 0.0f)
    {
        float p = out.animPhase + c.animScrub * deltaT;
        // fastFmod-equivalent for the [0, 1) wrap.
        p = p - floorf(p);
        out.animPhase = p;
    }
    if (c.playPauseAnim)
    {
        out.animSpeed = (out.animSpeed > 0.001f) ? 0.0f : defaultAnimSpeed;
    }
    if (c.resetAnim)
    {
        out.animPhase = 0.0f;
        out.animSpeed = 0.0f;
    }
    if (c.openAnim)
    {
        out.animPhase = 1.0f;
        out.animSpeed = 0.0f;
    }

    // Side-effects reported up the stack — caller actions them.
    out.didReloadTextures = c.reloadTextures;
    out.didExitViewer = c.exitViewer;

    return out;
}
} // namespace Poseidon
