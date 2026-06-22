#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/UI/Locale/Stringtable/Stringtable.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

using Poseidon::LocalizeString;

namespace Poseidon
{
class StringtableFunctions : public LocalizeStringFunctions
{
  public:
    RString LocalizeString(const char* str) override;

    StringtableFunctions() { ParamFile::SetDefaultLocalizeStringFunctions(this); }
};

// Meyers singleton - no global constructor
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
static StringtableFunctions& GStringtableFunctions()
{
    static StringtableFunctions instance;
    return instance;
}
#pragma clang diagnostic pop

void InitParamFileStringtable()
{
    // Force initialization of singleton
    (void)GStringtableFunctions();
}

RString StringtableFunctions::LocalizeString(const char* str)
{
    return ::LocalizeString(str);
}

} // namespace Poseidon
