#pragma once

#include <Poseidon/Foundation/Time/Time.hpp>
#include <Poseidon/World/Simulation/Animation/Animation.hpp>
#include <Poseidon/World/Scene/Object.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>


namespace Poseidon
{
class SurroundTracker
{
	Vector3 _lastPos;
	Foundation::Time _lastTime;
	float _value;

	public:
	SurroundTracker();

	void Update( const Object *who, Vector3Par pos, float radius, float minObstacle );

	float Track( const Object *who, Vector3Par pos, float radius, float minObstacle );
	float GetDensity() const {return _value;}
};

}  // namespace Poseidon
