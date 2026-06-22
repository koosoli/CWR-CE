#pragma once

#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

RString& GetFlashpointCfgInternal();

} // namespace Poseidon

// Macros for accessing config strings
#define FlashpointCfg Poseidon::GetFlashpointCfgInternal()
