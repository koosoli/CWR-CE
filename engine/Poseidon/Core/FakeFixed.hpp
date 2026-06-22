#pragma once
#include <cmath>
#include <cstdlib>


namespace Poseidon
{
typedef float Fixed;

// standard way to call explicit conversions:
#define fixed(x) (x)

#define fxToInt(x) toLargeInt(x)
#define fxToIntFloor(x) toLargeInt((x)-0.5f)
#define fxToIntCeil(x) toLargeInt((x)+0.5f)
#define fxToFloat(x) (x)

#define Fixed0 (0.0f)

#include "math.h"


} // namespace Poseidon
