
#include <Poseidon/World/Entities/Vehicles/GearBox.hpp>
#include <Poseidon/Core/Global.hpp>
#include <cmath>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Time/Time.hpp>

namespace Poseidon
{
using namespace Foundation;

GearBox::GearBox() : _gear(0), _gearWanted(0), _gearChangeTime(0) {}

void GearBox::SetGears(const AutoArray<float>& gears)
{
    _gears = gears;
}

void GearBox::ChangeGearUp(int gear, float time)
{
    Time chTime = Glob.time + time;
    if (_gearWanted >= gear)
    {
        if (_gearChangeTime > chTime)
        {
            _gearChangeTime = chTime;
        }
        return;
    }
    _gearWanted = gear;
    _gearChangeTime = chTime;
}
void GearBox::ChangeGearDown(int gear, float time)
{
    Time chTime = Glob.time + time;
    if (_gearWanted <= gear)
    {
        if (_gearChangeTime > chTime)
        {
            _gearChangeTime = chTime;
        }
        return;
    }
    _gearWanted = gear;
    _gearChangeTime = chTime;
}

bool GearBox::Change(float speedSize)
{
    int selGear = _gear;
    if (selGear < 1)
    {
        selGear = 1;
    }
    float spdGear = speedSize * _gears[selGear];
    while (selGear < _gears.Size() - 1 && spdGear >= 1)
    {
        selGear++;
        float selRpm = spdGear;
        float time = 1;
        if (selRpm > 1)
        {
            time = 1 / (1 + (selRpm - 1) * 4);
        }
        ChangeGearUp(selGear, time);
        spdGear = speedSize * _gears[selGear];
    }
    while (selGear > 1 && spdGear <= 0.6)
    {
        selGear--;
        float selRpm = 1.5 - spdGear;
        float time = 1;
        if (selRpm > 1)
        {
            time = 1 / (1 + (selRpm - 1) * 4);
        }
        ChangeGearDown(selGear, time);
        spdGear = speedSize * _gears[selGear];
    }
    if (_gearWanted != _gear)
    {
        if (Glob.time >= _gearChangeTime)
        {
            _gear = _gearWanted;
            return true;
        }
    }
    return false;
}

bool GearBox::Neutral()
{
    if (fabs(_gears[_gear]) < 1e-3)
    {
        return false;
    }
    int selGear;
    for (selGear = 0; selGear < _gears.Size(); selGear++)
    {
        if (fabs(_gears[selGear]) < 1e-3)
        {
            _gear = selGear;
            return true;
        }
    }
    Fail("No neutral");
    return false;
}

bool GearBox::Reverse()
{
    if (_gears[_gear] < 0)
    {
        return false;
    }
    int selGear;
    for (selGear = 0; selGear < _gears.Size(); selGear++)
    {
        if (_gears[selGear] < 0)
        {
            _gear = selGear;
            return true;
        }
    }
    Fail("No reverse");
    return false;
}

bool GearBox::Forward()
{
    if (_gears[_gear] > 0)
    {
        return false;
    }
    int selGear;
    for (selGear = 0; selGear < _gears.Size(); selGear++)
    {
        if (_gears[selGear] > 0)
        {
            if (_gear == selGear)
            {
                return false;
            }
            _gear = selGear;
            return true;
        }
    }
    Fail("No forward");
    return false;
}

} // namespace Poseidon
