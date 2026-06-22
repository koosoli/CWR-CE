#pragma once

#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <cstdint>

namespace Poseidon::Asset::Formats
{

// Trivially copyable POD structures for binary reading.
// Use aggregate initialization: Vector3{1.0f, 2.0f, 3.0f} or Vector3{} for zero-init.
struct Vector2
{
    float u = 0.0f, v = 0.0f;
};

struct Vector3
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
};

struct Vector4
{
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
};

struct Matrix3x3
{
    float data[9] = {}; // row-major, zero-initialized
};

struct Matrix4x3
{
    Vector3 rows[4]; // [aside, up, dir, pos]
};

struct BoundingBox
{
    Vector3 min, max;
};

struct BoundingSphere
{
    Vector3 center;
    float   radius = 0.0f;
};

struct ColorBGRA
{
    uint8_t b = 0, g = 0, r = 0, a = 255;

    static ColorBGRA fromUint32(uint32_t value)
    {
        ColorBGRA color;
        color.b = value & 0xFF;
        color.g = (value >> 8) & 0xFF;
        color.r = (value >> 16) & 0xFF;
        color.a = (value >> 24) & 0xFF;
        return color;
    }

    uint32_t toUint32() const { return (b) | (g << 8) | (r << 16) | (a << 24); }
};

inline void read(BinaryReader& reader, Vector2& vec)
{
    reader.read(vec.u);
    reader.read(vec.v);
}

inline void read(BinaryReader& reader, Vector3& vec)
{
    reader.read(vec.x);
    reader.read(vec.y);
    reader.read(vec.z);
}

inline void read(BinaryReader& reader, Vector4& vec)
{
    reader.read(vec.x);
    reader.read(vec.y);
    reader.read(vec.z);
    reader.read(vec.w);
}

inline void read(BinaryReader& reader, Matrix3x3& mat)
{
    for (int i = 0; i < 9; ++i)
        reader.read(mat.data[i]);
}

inline void read(BinaryReader& reader, Matrix4x3& mat)
{
    for (int i = 0; i < 4; ++i)
        read(reader, mat.rows[i]);
}

inline void read(BinaryReader& reader, BoundingBox& bbox)
{
    read(reader, bbox.min);
    read(reader, bbox.max);
}

inline void read(BinaryReader& reader, BoundingSphere& sphere)
{
    read(reader, sphere.center);
    reader.read(sphere.radius);
}

inline void read(BinaryReader& reader, ColorBGRA& color)
{
    uint32_t value = reader.read<uint32_t>();
    color           = ColorBGRA::fromUint32(value);
}

} // namespace Poseidon::Asset::Formats
