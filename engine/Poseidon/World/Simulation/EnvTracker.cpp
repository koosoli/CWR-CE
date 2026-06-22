#include <Poseidon/World/Simulation/EnvTracker.hpp>

#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Math/MathOpt.hpp>
#include <Poseidon/Foundation/Time/Time.hpp>

namespace Poseidon
{
SurroundTracker::SurroundTracker()
{
    _lastPos = VZero;
    _lastTime = TIME_MIN;
    _value = 0;
}

void SurroundTracker::Update(const Object* who, Vector3Par pos, float radius, float minObstacle)
{
    _lastPos = pos;
    _lastTime = Glob.time;
    // calculate density surroundings

    int xMin, xMax, zMin, zMax;
    ObjRadiusRectangle(xMin, xMax, zMin, zMax, pos, pos, radius);

    float total = 0;
    float radius2 = Square(radius);
    int x, z;
    for (x = xMin; x <= xMax; x++)
    {
        for (z = zMin; z <= zMax; z++)
        {
            const ObjectList& list = GLandscape->GetObjects(z, x);
            int n = list.Size();
            for (int i = 0; i < n; i++)
            {
                Object* obj = list[i];
                if (!obj || obj == who)
                {
                    continue;
                }
                if (obj->GetType() == Network)
                {
                    continue;
                }
                if (obj->GetType() == Temporary)
                {
                    continue;
                }
                if (obj->GetType() == TypeTempVehicle)
                {
                    continue;
                }
                if (!obj->Static())
                {
                    continue;
                }
                float objRadius = obj->GetRadius();
                if (objRadius < minObstacle)
                {
                    continue;
                }
                float dist2 = obj->Position().Distance2(pos);
                if (dist2 > radius2)
                {
                    continue;
                }

                saturateMax(objRadius, minObstacle * 4);

                float invDist = dist2 > 1 ? InvSqrt(dist2) : 1;
                float relSize = objRadius * invDist;
                total += relSize * 0.2;
            }
        }
    }
    saturateMin(total, 0.90);

    _value = total;
}

float SurroundTracker::Track(const Object* who, Vector3Par pos, float radius, float minObstacle)
{
    if (_lastPos.Distance2(pos) > Square(radius * 0.1))
    {
        Update(who, pos, radius, minObstacle);
    }
    return _value;
}

} // namespace Poseidon
