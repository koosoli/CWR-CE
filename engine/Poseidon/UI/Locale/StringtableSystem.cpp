#include <Poseidon/UI/Locale/StringtableSystem.hpp>

namespace Poseidon
{

extern bool g_stringtableSystemAvailable;

StringtableSystem& StringtableSystem::Instance()
{
    static StringtableSystem instance;
    return instance;
}

bool StringtableSystem::IsAvailable() const
{
    return g_stringtableSystemAvailable;
}

void StringtableSystem::Shutdown()
{
    g_stringtableSystemAvailable = false;
}

} // namespace Poseidon
