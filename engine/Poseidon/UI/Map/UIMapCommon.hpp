#pragma once

#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Containers/BoolArray.hpp>

// Shared declarations for uiMap split files


extern const float CameraZoom;
extern const float InvCameraZoom;
namespace Poseidon { int GetDaysInMonth(int year, int month); }

class CStatic;
class ControlsContainer;

namespace Poseidon
{
class ParamEntry;

CStatic* CreateStaticMap(ControlsContainer* parent, int idc, const ParamEntry& cls);
CStatic* CreateStaticMapMain(ControlsContainer* parent, int idc, const ParamEntry& cls);

void PositionToAA11(Vector3Val pos, char* buffer);
RString GetUserParams();
PackedBoolArray ListSelectedUnits();

static const float ptsPerSquareExp = 8;
static const float ptsPerSquareCost = 8;

} // namespace Poseidon

using ::Poseidon::GetUserParams;
using ::Poseidon::PositionToAA11;
