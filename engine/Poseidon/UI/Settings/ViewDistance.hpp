#pragma once

#include <algorithm>
#include <optional>

namespace Poseidon
{
// The three render distances the engine derives from one master view distance.
struct ViewDistances
{
    float view = 900.0f;    // tacticalZ / horizontZ — terrain + fog far plane
    float objects = 600.0f; // objectsZ — object cull distance
    float shadows = 250.0f; // shadowsZ — shadow caster / receiver distance
};

// Clamp + ratio parameters.  Defaults reproduce the original OFP derivation
// (objects = 2/3 of the view distance, shadows = 5/18) and the engine min/max
// clamps.  Passed in by value so the resolver has no dependency on engine /
// config globals and can be unit-tested in isolation.
struct ViewDistanceLimits
{
    float minView = 100.0f;
    float maxView = 5000.0f;
    float minObject = 100.0f;
    float objectCap = 3000.0f; // objects never exceed this
    float minShadow = 50.0f;
    float shadowCap = 500.0f;            // shadows never exceed this
    float objectRatio = 600.0f / 900.0f; // objects = objectRatio * view
    float shadowRatio = 250.0f / 900.0f; // shadows = shadowRatio * view
};

// Pure resolver: turns the configured / mission view-distance inputs into the
// three render distances.  No engine dependencies — the caller supplies the
// values, applies the result, and decides separately whether a debug-menu
// override replaces the engine-resolved view distance.
class ViewDistanceResolver
{
  public:
    // Master view distance: the mission's value when it is respected and
    // present, otherwise the engine/user preferred value.  Clamped to
    // [minView, maxView].
    static float EffectiveView(float preferredView, bool respectMission, const std::optional<float>& missionView,
                               const ViewDistanceLimits& limits = {})
    {
        const float v = (respectMission && missionView.has_value()) ? *missionView : preferredView;
        return Clamp(v, limits.minView, limits.maxView);
    }

    // Derive object + shadow distances from a master view distance.  Shadows
    // never exceed the object distance (no casters past where objects draw).
    static ViewDistances Derive(float view, const ViewDistanceLimits& limits = {})
    {
        view = Clamp(view, limits.minView, limits.maxView);
        const float objects = Clamp(view * limits.objectRatio, limits.minObject, limits.objectCap);
        float shadows = view * limits.shadowRatio;
        shadows = std::min(shadows, limits.shadowCap);
        shadows = std::max(shadows, limits.minShadow);
        shadows = std::min(shadows, objects);
        return ViewDistances{view, objects, shadows};
    }

    // EffectiveView followed by Derive — the full config/mission -> 3 distances.
    static ViewDistances Resolve(float preferredView, bool respectMission, const std::optional<float>& missionView,
                                 const ViewDistanceLimits& limits = {})
    {
        return Derive(EffectiveView(preferredView, respectMission, missionView, limits), limits);
    }

  private:
    static float Clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
};
} // namespace Poseidon
