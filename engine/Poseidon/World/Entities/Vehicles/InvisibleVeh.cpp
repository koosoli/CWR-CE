
#include <Poseidon/World/Entities/Vehicles/InvisibleVeh.hpp>
#include <Poseidon/AI/AI.hpp>

#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace Poseidon
{
InvisibleVehicleType::InvisibleVehicleType(const ParamEntry* param) : base(param)
{
    _scopeLevel = 1;
}

void InvisibleVehicleType::Load(const ParamEntry& par)
{
    base::Load(par);
}

void InvisibleVehicleType::InitShape()
{
    if (!_shape)
    {
        return;
    }
    _scopeLevel = 2;
    base::InitShape();
}

InvisibleVehicle::InvisibleVehicle(VehicleType* name) : base(name)
{
    _brain = new AIUnit(this);
}

void InvisibleVehicle::DrawDiags()
{
    Ref<Object> obj = new ObjectPlain(GScene->Preloaded(SphereModel), -1);
    Color color(1, 1, 0);
    obj->SetPosition(Position());
    obj->SetScale(3);
    obj->SetConstantColor(PackedColor(color));
    GLandscape->ShowObject(obj);
}

InvisibleVehicle::~InvisibleVehicle() = default;

void InvisibleVehicle::Simulate(float deltaT, SimulationImportance prec) {}

} // namespace Poseidon
