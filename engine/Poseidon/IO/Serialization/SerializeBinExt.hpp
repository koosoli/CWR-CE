#pragma once

#include <Poseidon/IO/Streams/SerializeBin.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>


namespace Poseidon
{
void operator << ( SerializeBinStream &s, Vector3 &data );
void operator << ( SerializeBinStream &s, Matrix3 &data );
void operator << ( SerializeBinStream &s, Matrix4 &data );

} // namespace Poseidon
