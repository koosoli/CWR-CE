#pragma once

#include <Poseidon/Input/InputContext.hpp>

namespace Poseidon
{

class AIUnit;
class Person;
class Transport;
class World;

enum class InputSeatContext
{
    NoPerson,
    OnFoot,
    Driver,
    Gunner,
    CommanderAsDriver,
    Commander,
    Cargo,
};

struct InputContextResolution
{
    InputContext context = InputContext::Spectator;
    Transport* transport = nullptr;
    InputSeatContext seat = InputSeatContext::NoPerson;
};

const char* InputContextName(InputContext ctx);
const char* InputSeatContextName(InputSeatContext seat);

Transport* ResolveInputTransport(AIUnit* unit);
InputContextResolution ResolveVehicleInputContext(Person* person, Transport* transport);
InputContextResolution ResolveGameplayInputContext(Person* person);

} // namespace Poseidon
