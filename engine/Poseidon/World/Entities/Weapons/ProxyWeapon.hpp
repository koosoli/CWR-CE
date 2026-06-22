#pragma once

#include <Poseidon/World/Entities/Vehicles/Vehicle.hpp>


namespace Poseidon
{
class ProxyWeaponType: public VehicleNonAIType
{
	typedef VehicleNonAIType base;

	Vector3 _cameraPos;

	public:
	ProxyWeaponType( const ParamEntry *param );

	void Load(const ParamEntry &par) override;
	void InitShape() override;

	Vector3Val GetCameraPos() const {return _cameraPos;}
};

class ProxyWeapon: public Vehicle
{
	typedef Vehicle base;

	public:
	ProxyWeapon( VehicleNonAIType *type, LODShapeWithShadow *shape );

	const ProxyWeaponType *Type() const
	{
		return static_cast<const ProxyWeaponType *>(GetNonAIType());
	}

	USE_CASTING(base)
};

class ProxySecWeapon: public ProxyWeapon
{
	typedef ProxyWeapon base;

	public:
	ProxySecWeapon( VehicleNonAIType *type, LODShapeWithShadow *shape );

	USE_CASTING(base)
};

class ProxyHandGun: public ProxyWeapon
{
	typedef ProxyWeapon base;

	public:
	ProxyHandGun( VehicleNonAIType *type, LODShapeWithShadow *shape );

	USE_CASTING(base)
};

enum CrewPosition {CPDriver,CPGunner,CPCommander,CPCargo};

class ProxyCrewType: public VehicleNonAIType
{
	typedef VehicleNonAIType base;
	CrewPosition _pos;

	public:
	ProxyCrewType(const ParamEntry *param);

	void Load(const ParamEntry &par) override;
	void InitShape() override;

	CrewPosition GetCrewPosition() const {return _pos;}

};

class ProxyCrew: public Vehicle
{
	typedef Vehicle base;

	public:
	ProxyCrew(VehicleNonAIType *type);

	const ProxyCrewType *Type() const
	{
		return static_cast<const ProxyCrewType *>(GetNonAIType());
	}

	USE_CASTING(base)
};

}  // namespace Poseidon
