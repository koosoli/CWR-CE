#pragma once

#include <Poseidon/Foundation/Math/Math3D.hpp>

namespace Poseidon
{
class ParamEntry;

struct LicensePlateTextTuning
{
    float widthScale = 0.8f;
    float horizontalOffset = -3.0f;
    float verticalOffset = 0.5f;
    float surfaceOffset = 0.01f;
    float softness = 0.0f;
};

struct LicensePlateTextFrame
{
    Vector3 origin;
    Vector3 up;
    Vector3 right;
};

LicensePlateTextTuning GetLicensePlateTextTuning();
void SetLicensePlateTextTuning(const LicensePlateTextTuning& tuning);
void ResetLicensePlateTextTuning();
bool LoadLicensePlateTextTuningFromConfig(const ParamEntry* cfg);

LicensePlateTextFrame BuildLicensePlateTextFrame(Vector3 center, Vector3 normal, Vector3 modelUp, float size,
                                                 const LicensePlateTextTuning& tuning);

} // namespace Poseidon
