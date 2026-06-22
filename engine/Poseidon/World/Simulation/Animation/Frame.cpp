#include <Poseidon/Core/Visual.hpp>
#include <cmath>
#include <Poseidon/Foundation/Math/Math3DP.hpp>

namespace Poseidon
{
bool Frame::Equal(const Frame& with) const
{
    if (fabs(Position()[0] - with.Position()[0]) > 1e-3)
    {
        return false;
    }
    if (fabs(Position()[1] - with.Position()[1]) > 1e-3)
    {
        return false;
    }
    if (fabs(Position()[2] - with.Position()[2]) > 1e-3)
    {
        return false;
    }
    for (int i = 0; i < 3; i++)
    {
        if (fabs(Orientation()(i, 0) - with.Orientation()(i, 0)) > 1e-3)
        {
            return false;
        }
        if (fabs(Orientation()(i, 1) - with.Orientation()(i, 1)) > 1e-3)
        {
            return false;
        }
        if (fabs(Orientation()(i, 2) - with.Orientation()(i, 2)) > 1e-3)
        {
            return false;
        }
    }
    return true;
}

} // namespace Poseidon
