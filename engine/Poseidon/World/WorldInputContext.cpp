#include <Poseidon/World/WorldInputContext.hpp>

#include <Poseidon/AI/AI.hpp>
#include <Poseidon/World/World.hpp>
#include <Poseidon/World/Entities/Infantry/Person.hpp>
#include <Poseidon/World/Entities/Vehicles/Air/Airplane.hpp>
#include <Poseidon/World/Entities/Vehicles/Air/Helicopter.hpp>
#include <Poseidon/World/Entities/Vehicles/Ground/Car.hpp>
#include <Poseidon/World/Entities/Vehicles/Ground/Tank.hpp>
#include <Poseidon/World/Entities/Vehicles/Misc/Ship.hpp>
#include <Poseidon/World/Entities/Vehicles/Transport.hpp>

namespace Poseidon
{

const char* InputContextName(InputContext ctx)
{
    switch (ctx)
    {
        case InputContext::Menu:
            return "Menu";
        case InputContext::Infantry:
            return "Infantry";
        case InputContext::CarDriver:
            return "CarDriver";
        case InputContext::TankDriver:
            return "TankDriver";
        case InputContext::TankGunner:
            return "TankGunner";
        case InputContext::HeliPilot:
            return "HeliPilot";
        case InputContext::PlanePilot:
            return "PlanePilot";
        case InputContext::ShipDriver:
            return "ShipDriver";
        case InputContext::Gunner:
            return "Gunner";
        case InputContext::Spectator:
            return "Spectator";
        case InputContext::Map:
            return "Map";
        case InputContext::Chat:
            return "Chat";
        case InputContext::Editor:
            return "Editor";
        case InputContext::Count:
            break;
    }
    return "Unknown";
}

const char* InputSeatContextName(InputSeatContext seat)
{
    switch (seat)
    {
        case InputSeatContext::NoPerson:
            return "no-person";
        case InputSeatContext::OnFoot:
            return "on-foot";
        case InputSeatContext::Driver:
            return "driver";
        case InputSeatContext::Gunner:
            return "gunner";
        case InputSeatContext::CommanderAsDriver:
            return "commander-as-driver";
        case InputSeatContext::Commander:
            return "commander";
        case InputSeatContext::Cargo:
            return "cargo";
    }
    return "unknown";
}

Transport* ResolveInputTransport(AIUnit* unit)
{
    if (!unit)
        return nullptr;
    if (Transport* transport = unit->GetVehicleIn())
        return transport;
    return dyn_cast<Transport>(unit->GetVehicle());
}

InputContextResolution ResolveVehicleInputContext(Person* person, Transport* transport)
{
    InputContextResolution result;
    result.transport = transport;

    if (!person)
    {
        result.context = InputContext::Spectator;
        result.seat = InputSeatContext::NoPerson;
        return result;
    }

    if (!transport)
    {
        result.context = InputContext::Infantry;
        result.seat = InputSeatContext::OnFoot;
        return result;
    }

    if (person == transport->Gunner())
    {
        result.context = dynamic_cast<Tank*>(transport) ? InputContext::TankGunner : InputContext::Gunner;
        result.seat = InputSeatContext::Gunner;
        return result;
    }

    if (person == transport->Driver() || (person == transport->Commander() && transport->Type()->DriverIsCommander()))
    {
        if (dynamic_cast<Helicopter*>(transport))
            result.context = InputContext::HeliPilot;
        else if (dynamic_cast<Airplane*>(transport))
            result.context = InputContext::PlanePilot;
        else if (dynamic_cast<Ship*>(transport))
            result.context = InputContext::ShipDriver;
        else if (dynamic_cast<Tank*>(transport))
            result.context = InputContext::TankDriver;
        else
            result.context = InputContext::CarDriver;

        result.seat = person == transport->Commander() ? InputSeatContext::CommanderAsDriver : InputSeatContext::Driver;
        return result;
    }

    if (person == transport->Commander())
    {
        result.context = InputContext::Gunner;
        result.seat = InputSeatContext::Commander;
        return result;
    }

    result.context = InputContext::Infantry;
    result.seat = InputSeatContext::Cargo;
    return result;
}

InputContextResolution ResolveGameplayInputContext(Person* person)
{
    if (!person)
        return {};

    AIUnit* unit = person->Brain();
    if (!unit)
    {
        InputContextResolution result;
        result.context = InputContext::Infantry;
        result.seat = InputSeatContext::OnFoot;
        return result;
    }

    return ResolveVehicleInputContext(person, ResolveInputTransport(unit));
}

InputContextResolution World::ResolveInputContextResolution() const
{
    InputContextResolution result;

    if (_editor)
    {
        result.context = InputContext::Editor;
        return result;
    }
    if (HasOptions())
    {
        result.context = InputContext::Menu;
        return result;
    }
    if (_showMap)
    {
        result.context = InputContext::Map;
        return result;
    }
    if (!PlayerManual())
    {
        result.context = InputContext::Spectator;
        return result;
    }

    if (AIUnit* focus = FocusOn())
    {
        if (Person* person = focus->GetPerson())
            return ResolveGameplayInputContext(person);
    }

    return ResolveGameplayInputContext(PlayerOn());
}

InputContext World::ResolveInputContext() const
{
    return ResolveInputContextResolution().context;
}

} // namespace Poseidon
