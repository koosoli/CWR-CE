#pragma once

// TargetId / TargetType / VehicleWithAI, extracted so the ~60 AI/World/entity
// headers that reference an AI target by OLink can include a leaf header instead
// of the full EntityAI.hpp (which cycles). TargetType is EntityAI everywhere
// (VehicleWithAI is just a typedef alias), so this is unambiguous.

#include <Poseidon/Foundation/Types/LLinks.hpp>


namespace Poseidon
{
class EntityAI;
typedef EntityAI VehicleWithAI;
typedef EntityAI TargetType;
typedef LLink<TargetType> TargetId;

}  // namespace Poseidon
