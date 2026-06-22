#pragma once

#include <Poseidon/World/Simulation/Animation/Animation.hpp>

namespace Poseidon { class Clock; }
namespace Poseidon
{
using Poseidon::Clock;

class Indicator: public AnimationRotation
{
	typedef AnimationRotation base;
protected:
	float _fullAngle;
	float _minValue;
	float _invValuesRange;

public:
	Indicator();
	void Init(LODShape *shape, const char *name, const char *axis);
	void Init(LODShape *shape, const ParamEntry &par);
	void SetFullAngle(float angle) {_fullAngle = angle;}
	void SetRange(float minValue, float maxValue)
	{
		_minValue = minValue;
		float valuesRange = maxValue - minValue;
		_invValuesRange = valuesRange != 0 ? 1.0 / valuesRange : 0;
	}

	void SetValue(LODShape *shape, int level, float value) const;
	void SetValue
	(
		LODShape *shape, int level, float value,
		Matrix4Par anim
	) const;
	void Restore(LODShape *shape, int level) const;

	void GetRotationForValue(Matrix4 &mat, int level, float value) const;
};

class IndicatorWatch
{
protected:
	AnimationRotation _hour;
	AnimationRotation _minute;
	bool _reversed;

public:
	IndicatorWatch() {}
	void Init(LODShape *shape, const char *hour, const char *minute, const char *axis, bool reversed);
	void Init(LODShape *shape, const ParamEntry &par);

	void SetTime(LODShape *shape, int level, Poseidon::Clock &time) const;
	void Restore(LODShape *shape, int level) const;
};

} // namespace Poseidon
