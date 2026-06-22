#pragma once

#if _ENABLE_CHEATS

#include <Poseidon/Foundation/Enums/EnumNames.hpp>

namespace Poseidon::Dev
{

#define DIAG_ENABLE_ENUM(type,prefix,XX) \
	XX(type, prefix, CostMap) \
	XX(type, prefix, LockMap) \
	XX(type, prefix, Combat) \
	XX(type, prefix, Force) \
	XX(type, prefix, Animation) \
	XX(type, prefix, Dammage) \
	XX(type, prefix, Collision) \
	XX(type, prefix, Transparent) \
	XX(type, prefix, Sound) \
	XX(type, prefix, Path)

DECLARE_ENUM(DiagEnable,DE,DIAG_ENABLE_ENUM)

extern int DiagMode;

} // namespace Poseidon::Dev

#define CHECK_DIAG(x) ( ::Poseidon::Dev::DiagMode&(1<<(x)) )

#endif
