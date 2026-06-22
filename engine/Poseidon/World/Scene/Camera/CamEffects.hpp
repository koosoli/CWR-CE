#pragma once

#include <Poseidon/World/World.hpp>

namespace Poseidon
{
DEFINE_ENUM_BEG(CamEffectName)
	CamEffectNone,
	
	CamEffectStatic, // static
	CamEffectTracking, // static with zoom
	CamEffectFollowNear, // near following
	CamEffectFollowFar, // near following
	CamEffectExternal,
	CamEffectExternalFixed,
	CamEffectAligned, // aligned with vehicle
	CamEffectZoomIn, // zoom and show for a while
	CamEffectZoomOut, // show for a while and zoom
	CamEffectBoomAndZoom, // zoom in, show, zoom out
	CamEffectTransition, // transition from current effect (if any)
	CamEffectTerminate, // terminate intro/outro
	
	NCamEffects
DEFINE_ENUM_END(CamEffectName)

DEFINE_ENUM_BEG(CamEffectPosition)
	CamEffectTop,
	CamEffectLeft,CamEffectRight,
	CamEffectFront,CamEffectBack,

	CamEffectLeftFront,CamEffectRightFront,
	CamEffectLeftBack,CamEffectRightBack,

	CamEffectLeftTop,CamEffectRightTop,
	CamEffectFrontTop,CamEffectBackTop,

	CamEffectBottom,
	
	NCamEffectPositions
DEFINE_ENUM_END(CamEffectPosition)

class CameraEffect;

CameraEffect *CreateCameraEffect
(
	Object *obj, const char *name, CamEffectPosition pos
);

CameraEffect *CreateCameraEffect
(
	Object *obj, const char *name, CamEffectPosition pos, bool infinite
);

}  // namespace Poseidon
