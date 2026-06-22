#pragma once

#include <Poseidon/AI/VehicleAI.hpp>


namespace Poseidon
{
class InvisibleVehicleType: public VehicleType
{
	typedef VehicleType base;

	friend class InvisibleVehicle;

	public:
	InvisibleVehicleType( const ParamEntry *param );
	void Load(const ParamEntry &par) override;
	void InitShape() override;
};

class InvisibleVehicle: public Person
{
	typedef Person base;

	public:
	const InvisibleVehicleType *Type() const
	{
		return static_cast<const InvisibleVehicleType *>(GetType());
	}

	InvisibleVehicle( VehicleType *name );
	~InvisibleVehicle() override;

	bool Invisible() const override {return true;}
	
	void DrawDiags() override;

	void ResetMovement( float speed, int action) override {}

	void Simulate(float deltaT, SimulationImportance prec) override;
};

}  // namespace Poseidon
