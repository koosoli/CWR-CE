#include <Poseidon/UI/Locale/StringtableExt.hpp>

#include <Poseidon/Foundation/Modules/Modules.hpp>

// IDS_X variables are global (declared as `extern int IDS_X;` in
// StringtableExt.hpp without a namespace wrap).
#define STRING(x) int IDS_##x;
#include <Poseidon/IO/Serialization/StringIds.hpp>
#undef STRING

namespace Poseidon
{

INIT_MODULE(StringtableExt, 1)
{
    // StringtableSystem is an opt-in subsystem — apps that ship no
    // bin/stringtable.csv (Tetris, utility tools) skip the load and
    // leave `g_stringtableSystemAvailable` false.  In that case there's
    // nothing to look up, so don't run the IDS_ registration loop:
    // every Register() call would warn "key not found in stringtable"
    // and the resulting IDS_X = -1 would just be a noisier zero.  The
    // BSS-zero default for IDS_X is already correct for "no i18n".
    extern bool g_stringtableSystemAvailable;
    if (!g_stringtableSystemAvailable)
        return;

#define STRING(x) IDS_##x = RegisterString("STR_" #x);
#include <Poseidon/IO/Serialization/StringIds.hpp>
#undef STRING
}

} // namespace Poseidon
