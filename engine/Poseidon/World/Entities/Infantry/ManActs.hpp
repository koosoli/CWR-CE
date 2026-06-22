#pragma once

#include <Poseidon/Foundation/Types/EnumDecl.hpp>

namespace Poseidon
{
DEFINE_ENUM_BEG(ManAction)
	#define ACTION(x) ManAct##x,
	#include <Poseidon/World/Entities/Infantry/ManActions.hpp>

	ManActNormN,
	// ManActNoIncrement keeps the enum counter from advancing past ManActNormN
	ManActNoIncrement = ManActNormN-1,

	ManActN

	#undef ACTION
DEFINE_ENUM_END(ManAction)

}  // namespace Poseidon
