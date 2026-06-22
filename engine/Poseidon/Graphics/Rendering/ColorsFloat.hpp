#pragma once

#define R_EYE 0.299f
#define G_EYE 0.587f
#define B_EYE 0.114f

// passing by reference preferred
#define ColorVal const ColorP&

#include <Poseidon/Foundation/Math/MathDefs.hpp>
#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
namespace Poseidon
{
class ColorP
{
    // color used for shading calculations
  private:
    // valid color is in range <0,1)
    float _r, _g, _b, _a;

  public:
    __forceinline ColorP() {}             // default is uninitialized
    __forceinline ColorP(enum _noInit) {} // default is uninitialized
    __forceinline ColorP(float r, float g, float b)
    {
        _r = r;
        _g = g;
        _b = b;
        _a = 1;
    }
    __forceinline ColorP(float r, float g, float b, float a)
    {
        _r = r;
        _g = g;
        _b = b;
        _a = a;
    }
    explicit ColorP(long rgb)
    {
        _a = ((rgb >> 24) & 0xff) * (1 / 255.0f);
        _r = ((rgb >> 16) & 0xff) * (1 / 255.0f);
        _g = ((rgb >> 8) & 0xff) * (1 / 255.0f);
        _b = ((rgb >> 0) & 0xff) * (1 / 255.0f);
    }
    __forceinline float R() const { return _r; }
    __forceinline float G() const { return _g; }
    __forceinline float B() const { return _b; }
    __forceinline float A() const { return _a; }
    __forceinline int R8() const { return toInt(_r * 255); }
    __forceinline int G8() const { return toInt(_g * 255); }
    __forceinline int B8() const { return toInt(_b * 255); }
    __forceinline int A8() const { return toInt(_a * 255); }
    __forceinline void SetA(float a) { _a = a; }

    ColorP operator*(ColorVal op) const { return ColorP(_r * op._r, _g * op._g, _b * op._b, _a * op._a); }
    ColorP operator*(float c) const { return ColorP(_r * c, _g * c, _b * c, _a * c); }
    __forceinline ColorP& operator+=(ColorVal op)
    {
        _r += op._r;
        _g += op._g;
        _b += op._b;
        _a += op._a;
        return *this;
    }

    ColorP operator+(ColorVal op) const { return ColorP(_r + op._r, _g + op._g, _b + op._b, _a + op._a); }
    ColorP operator-(ColorVal op) const { return ColorP(_r - op._r, _g - op._g, _b - op._b, _a - op._a); }
    float Brightness() const { return _r * R_EYE + _g * G_EYE + _b * B_EYE; }

    void Saturate()
    {
        saturateAbove(_r, 1.0f);
        saturateAbove(_g, 1.0f);
        saturateAbove(_b, 1.0f);
        // alpha never overflows
        saturate(_a, 0.0f, 1.0f);
    }
    void SaturateMinMax()
    {
        saturate(_r, 0.0f, 1.0f);
        saturate(_g, 0.0f, 1.0f);
        saturate(_b, 0.0f, 1.0f);
        saturate(_a, 0.0f, 1.0f);
    }
};

extern const ColorP HBlackP;
extern const ColorP HWhiteP;

class PackedColor
{ // packed color - for efficient storing, no calculations
    DWORD _value;

  public:
    __forceinline PackedColor() {}
    __forceinline PackedColor(int r, int g, int b, int a) // no clipping
    {
        _value = (a << 24) | (r << 16) | (g << 8) | b;
    }
    __forceinline explicit PackedColor(DWORD value) { _value = value; }
    explicit PackedColor(ColorVal color) // full clipping
    {                                    // convert with saturation
        int r = toInt(color.R() * 255);
        int g = toInt(color.G() * 255);
        int b = toInt(color.B() * 255);
        int a = toInt(color.A() * 255);
        saturate(r, 0, 255);
        saturate(g, 0, 255);
        saturate(b, 0, 255);
        saturate(a, 0, 255);
        _value = (a << 24) | (r << 16) | (g << 8) | b;
    }
    void SetA8(int val) { _value = (_value & 0xffffff) | (val << 24); }
    __forceinline int A8() const { return (_value >> 24) & 0xff; }
    __forceinline int R8() const { return (_value >> 16) & 0xff; }
    __forceinline int G8() const { return (_value >> 8) & 0xff; }
    __forceinline int B8() const { return (_value >> 0) & 0xff; }
    operator ColorP() const
    {
        return ColorP(R8() * (1.0f / 255), G8() * (1.0f / 255), B8() * (1.0f / 255), A8() * (1.0f / 255));
    }
    __forceinline operator DWORD() const { return _value; }
    friend PackedColor PackedColorRGB(ColorVal rgb, int a);
    friend PackedColor PackedColorRGB(PackedColor rgb, int a);
};

inline PackedColor PackedColorRGB(ColorVal color, int a = 255)
{
    int r = toInt(color.R() * 255);
    int g = toInt(color.G() * 255);
    int b = toInt(color.B() * 255);
    saturate(r, 0, 255);
    saturate(g, 0, 255);
    saturate(b, 0, 255);
    PackedColor ret;
    ret._value = (a << 24) | (r << 16) | (g << 8) | b;
    return ret;
}

inline PackedColor PackedColorRGB(PackedColor color, int a = 255)
{
    PackedColor ret;
    ret._value = (color._value & 0xffffff) | (a << 24);
    return ret;
}

#define PackedWhite PackedColor(0xffffffff)
#define PackedBlack PackedColor(0xff000000)

// binary copy will do

} // namespace Poseidon

using Poseidon::ColorP;
using Poseidon::HBlackP;
using Poseidon::PackedColor;
