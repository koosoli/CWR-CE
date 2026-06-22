#pragma once

#include <Poseidon/Foundation/Types/EnumDecl.hpp>

namespace Poseidon
{
DEFINE_ENUM_BEG(MoveFinishF)
	MFNull,
	MFGetIn,
	MFReload,
	MFThrowGrenade,
	MFUIAction,
	MFDead,
	NMoveFinish
DEFINE_ENUM_END(MoveFinishF)

}  // namespace Poseidon
