#pragma once

#ifdef _KNI
#include <Poseidon/Graphics/Rendering/ColorsK.hpp>
#define Color ColorK
#define HWhite HWhiteK
#define HBlack HBlackK
#else
#include <Poseidon/Graphics/Rendering/ColorsFloat.hpp>
#define Color ColorP
#define HWhite HWhiteP
#define HBlack HBlackP
#endif
