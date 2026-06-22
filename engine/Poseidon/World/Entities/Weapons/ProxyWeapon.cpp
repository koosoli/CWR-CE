
#include <Poseidon/World/Entities/Vehicles/Vehicle.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>

#include <Poseidon/World/Entities/Weapons/ProxyWeapon.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace Poseidon
{
ProxyWeaponType::ProxyWeaponType(const ParamEntry* param) : base(param) {}

void ProxyWeaponType::Load(const ParamEntry& par)
{
    base::Load(par);
}

void ProxyWeaponType::InitShape()
{
    base::InitShape();

    // get weapon positions
    Shape* mem = _shape->MemoryLevel();
    if (mem)
    {
        if (mem->FindNamedSel("pilot") >= 0)
        {
            _cameraPos = mem->NamedPosition("pilot");
        }
        else
        {
            _cameraPos = Vector3(0.193, 0.166, 0) - _shape->BoundingCenter();
        }
    }
}

DEFINE_CASTING(ProxyWeapon)

ProxyWeapon::ProxyWeapon(VehicleNonAIType* type, LODShapeWithShadow* shape) : base(shape, type, -1) {}

DEFINE_CASTING(ProxySecWeapon)

ProxySecWeapon::ProxySecWeapon(VehicleNonAIType* type, LODShapeWithShadow* shape) : base(type, shape) {}

DEFINE_CASTING(ProxyHandGun)

ProxyHandGun::ProxyHandGun(VehicleNonAIType* type, LODShapeWithShadow* shape) : base(type, shape) {}

ProxyCrewType::ProxyCrewType(const ParamEntry* param) : base(param) {}

void ProxyCrewType::Load(const ParamEntry& par)
{
    base::Load(par);
    int pos = par >> "crewPosition";
    _pos = (CrewPosition)pos;
}

void ProxyCrewType::InitShape()
{
    base::InitShape();
}

DEFINE_CASTING(ProxyCrew)

ProxyCrew::ProxyCrew(VehicleNonAIType* type) : base(type->GetShape(), type, -1) {}

} // namespace Poseidon
