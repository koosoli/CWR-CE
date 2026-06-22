#pragma once


namespace Poseidon
{
namespace AspectRatio
{
enum DisplayStyle
{
    Modern = 0,
    Legacy,
    DisplayStyleCount,
};

enum UltrawideClamp
{
    ClampOff = 0,
    Clamp21x9,
    Clamp16x9,
    UltrawideClampCount,
};

enum Preset
{
    Auto = 0,
    Ratio4x3,
    Ratio5x4,
    Ratio16x10,
    Ratio15x9,
    Ratio16x9,
    Ratio21x9,
    Ratio32x9,
    Custom,
    Count,
};

struct PresetDefinition
{
    Preset preset;
    const char* label;
    float ratio;
    int width;
    int height;
};

struct Settings
{
    float leftFOV;
    float topFOV;
    float uiTopLeftX;
    float uiTopLeftY;
    float uiBottomRightX;
    float uiBottomRightY;
    // 3D world render rectangle as fractions of the full window (0..1).
    // Default (0,0,1,1) = full window (no crop).  A centered sub-rect
    // (e.g. left=0.1, right=0.9) is the pillarbox/noodle crop: the world
    // renders only inside it, the FOV matches its aspect (no stretch),
    // and the periphery is left for black bars.
    float worldLeft = 0.0f;
    float worldTop = 0.0f;
    float worldRight = 1.0f;
    float worldBottom = 1.0f;
};

struct PolicyInput
{
    int viewportWidth = 0;
    int viewportHeight = 0;
    DisplayStyle style = Modern;
    UltrawideClamp ultrawideClamp = Clamp21x9;
};

struct PolicyResult
{
    float viewportRatio = 0.0f;
    float effectiveRatio = 0.0f;
    Settings settings{};
};

const PresetDefinition* PresetDefinitions();
int PresetDefinitionCount();
const PresetDefinition& GetPresetDefinition(Preset preset);

float SanitizeRatio(float ratio);
float RatioFromDimensions(int width, int height);
void SuggestDimensions(float ratio, int& width, int& height);
float RatioForClamp(UltrawideClamp clamp);
Settings BuildSettingsForRatio(float ratio);
PolicyResult ResolvePolicy(const PolicyInput& input);

// Factory policy used for defaults and migration: Adaptive + 21:9.  On
// standard (<=16:9) viewports this fills the screen edge to edge,
// undistorted; on ultrawide it keeps the HUD readable by default.
PolicyInput SafeDefaultPolicy(int viewportWidth, int viewportHeight);

Settings ResolveSettings(Preset preset, int width, int height, const Settings& legacySettings, float customRatio = 0.0f);
Preset InferPresetFromSettings(const Settings& settings);
bool MatchesPreset(float ratio, Preset preset, float tolerance = 0.02f);

// Live experiment controls — driven by the dev-panel Aspect tab and the
// tri verbs so the aspect behaviour can be tuned in a running game
// without going through the Options menu.  When overrideEnabled is
// false the normal display.cfg-driven policy applies and these are
// ignored.  Global single instance via Live().
struct LiveControls
{
    bool overrideEnabled = false;
    DisplayStyle style = Modern;
    UltrawideClamp clamp = Clamp21x9;
    // Crop the 3D world to the clamp band + black bars (whole frame
    // locked to the band).  When false the world fills the viewport.
    bool pillarbox = false;
    // Keep the 2D UI/HUD centered in the clamp band even when the world
    // is full-width.  (No effect under pillarbox, which already bands UI.)
    bool hudClamp = true;
    // Bypass clamp entirely and crop the world to an explicit rect
    // (the "noodle" sliders).  Fractions of the full window.
    bool manualRect = false;
    float rectL = 0.0f;
    float rectT = 0.0f;
    float rectR = 1.0f;
    float rectB = 1.0f;
};

LiveControls& Live();

// Pure resolver for the live controls: produces FOV + UI insets + world
// crop rect for a given viewport.  The world FOV always matches the
// world rect's aspect, so a soldier is the same pixel size whether the
// world is full-width or cropped — only the visible extent changes.
Settings ResolveLive(const LiveControls& controls, int viewportWidth, int viewportHeight);

// Pillarbox bar width (in pixels) needed when a 4:3-designed full-
// screen overlay model (cutscene CinemaBorder, etc.) is drawn on a
// wider-than-4:3 viewport.  The overlay's 4/3 scaling means it fills
// only the central band; lateral strips of (viewportWidth - centerW)/2
// pixels each are uncovered.  This helper returns the width of ONE
// such strip (left == right).  Returns 0 on 4:3 or narrower viewports.
//
// Pure function — no graphics-engine dependency.  Callers paint the
// black quads themselves using whatever 2D primitive their engine
// exposes.
float LateralPillarboxWidth(int viewportWidth, int viewportHeight);

// Global on/off for the widescreen pillarbox treatment applied to
// 4:3-designed overlay models (cutscene bars, binocular vignette,
// vehicle gunsight scopes, NV goggle frame).  Defaults true.  When
// false, the lateral 3D scene leak is left visible — useful for
// users who want stretch / no-clamp behavior, or for tests that
// want to verify the un-pillarboxed render path.
//
// Object::DrawWidescreenPillarbox consults this; individual call
// sites just call the helper unconditionally and let the toggle
// decide.
bool ArePillarboxBarsEnabled();
void SetPillarboxBarsEnabled(bool enabled);

// "Are we inside actual gameplay?" — set true when the world enters
// a real mission (Arcade / Netware), false on the main-menu intro
// cutscene (GModeIntro) and at shutdown.  Owned by World; queried
// by Object::DrawWidescreenPillarbox.
//
// Why separate from the user-preference toggle: the pillarbox is
// only meaningful when a 4:3-designed dark-around overlay is on
// screen, which only happens during gameplay.  The menu intro also
// routes through CameraEffect::Draw (random cutscene behind the
// menu) but its 3D scene is meant to extend full-width — painting
// lateral bars there pillarboxes the menu background.  Folding the
// gate into the helper means every caller (CameraEffect, Binocs,
// NV goggles, vehicle gunsight) inherits the correct behavior with
// zero per-site branching.
//
// Defaults false (we boot into the menu intro, not into a mission).
bool IsGameplayActive();
void SetGameplayActive(bool active);
} // namespace AspectRatio

}  // namespace Poseidon
