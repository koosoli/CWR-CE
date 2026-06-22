#pragma once

#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon::Foundation
{
RString FormatNumber(int number);

// printf-style formatting into an RString
RString Format(const char *format, ...);

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::Format;
using ::Poseidon::Foundation::FormatNumber;
