#pragma once

#include <Poseidon/UI/Locale/Stringtable/Stringtable.hpp>

#define STRING(x) extern int IDS_##x;
#include <Poseidon/IO/Serialization/StringIds.hpp>
#undef STRING


