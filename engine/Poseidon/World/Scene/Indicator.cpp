#include <Poseidon/World/Scene/Indicator.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/Core/Global.hpp>
#include <math.h>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Math/MathDefs.hpp>
#include <Poseidon/Foundation/Math/MathOpt.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

Indicator::Indicator()
{
    _fullAngle = 0;
    _minValue = 0;
    _invValuesRange = 0;
}

void Indicator::Init(LODShape* shape, const char* name, const char* axis)
{
    base::Init(shape, name, nullptr, axis, nullptr, false);
}

void Indicator::Init(LODShape* shape, const ParamEntry& par)
{
    RString selection = par >> "selection";
    RString axis = par >> "axis";
    Init(shape, selection, axis);
    SetFullAngle(HDegree(par >> "angle"));
    SetRange(par >> "min", par >> "max");
}

void Indicator::SetValue(LODShape* shape, int level, float value, Matrix4Par anim) const
{
    float coef = (value - _minValue) * _invValuesRange;
    saturate(coef, 0, 1);
    float angle = coef * _fullAngle;

    if (_selection[level] < 0)
    {
        return;
    }
    Matrix4 transform;
    GetRotation(transform, angle, level);
    transform = anim * transform;

    Transform(shape, transform, level);
}

void Indicator::SetValue(LODShape* shape, int level, float value) const
{
    float coef = (value - _minValue) * _invValuesRange;
    saturate(coef, 0, 1);
    float angle = coef * _fullAngle;
    base::Rotate(shape, angle, level);
}

void Indicator::GetRotationForValue(Matrix4& mat, int level, float value) const
{
    float coef = (value - _minValue) * _invValuesRange;
    saturate(coef, 0, 1);
    float angle = coef * _fullAngle;
    base::GetRotation(mat, angle, level);
}

void Indicator::Restore(LODShape* shape, int level) const
{
    base::Restore(shape, level);
}

void IndicatorWatch::Init(LODShape* shape, const char* hour, const char* minute, const char* axis, bool reversed)
{
    _hour.Init(shape, hour, nullptr, axis, nullptr, false);
    _minute.Init(shape, minute, nullptr, axis, nullptr, false);
    _reversed = reversed;
}

void IndicatorWatch::Init(LODShape* shape, const ParamEntry& par)
{
    RString hour = par >> "hour";
    RString minute = par >> "minute";
    RString axis = par >> "axis";
    bool reversed = par >> "reversed";
    Init(shape, hour, minute, axis, reversed);
}

void IndicatorWatch::SetTime(LODShape* shape, int level, Clock& time) const
{
    float timeOfDay = time.GetTimeOfDay();
    if (timeOfDay >= 1.0)
    {
        timeOfDay--;
    }

    float angle = 4.0 * H_PI * timeOfDay;
    if (_reversed)
    {
        angle = -angle;
    }
    _hour.Rotate(shape, angle, level);

    timeOfDay = fmod(24.0 * timeOfDay, 1.0);
    angle = 2.0 * H_PI * timeOfDay;
    if (_reversed)
    {
        angle = -angle;
    }
    _minute.Rotate(shape, angle, level);
}

void IndicatorWatch::Restore(LODShape* shape, int level) const
{
    _hour.Restore(shape, level);
    _minute.Restore(shape, level);
}

} // namespace Poseidon
