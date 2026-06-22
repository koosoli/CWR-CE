#pragma once


#include <Poseidon/Core/Types.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
namespace Poseidon
{
ClipFlags NeedsClipping
(
	const Matrix4 &transform, const Camera &camera, Vector3Par point
);

} // namespace Poseidon
