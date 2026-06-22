#pragma once

#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Time/Time.hpp>


namespace Poseidon
{
class GearBox
{
	AutoArray<float> _gears;
	int _gear;
	int _gearWanted;
	Foundation::Time _gearChangeTime;

	protected:
	void ChangeGearUp( int gear, float time=1.0 );
	void ChangeGearDown( int gear, float time=1.0 );

	public:
	GearBox();
	void SetGears( const AutoArray<float> &gears );
	bool Change( float speed );
	bool Neutral();
	bool Forward();
	bool Reverse();

	int Gear() const {return _gear;}
	float Ratio() const {return _gears[_gear];}
};

}  // namespace Poseidon
