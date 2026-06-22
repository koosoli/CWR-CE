
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/IO/PreprocC/PreprocC.hpp>

// Meyers singleton - no global constructor
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"

namespace Poseidon
{
static CPreprocessorFunctions& GCPreprocessorFunctions()
{
    static CPreprocessorFunctions instance;
    return instance;
}
#pragma clang diagnostic pop

// Static member initialization with function call - requires global constructor in C++14
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
PreprocessorFunctions* ParamFile::_defaultPreprocFunctions = &GCPreprocessorFunctions();
#pragma clang diagnostic pop

} // namespace Poseidon
