#pragma once

#include <Poseidon/Foundation/Types/EnumDecl.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmicrosoft-enum-forward-reference"
#ifndef _DECL_ENUM_LSError
#define _DECL_ENUM_LSError
namespace Poseidon
{
DECL_ENUM(LSError)
}
#endif
using Poseidon::LSError;
#pragma clang diagnostic pop

class ParamArchive;

// Interface for classes with serialization.
class SerializeClass
{
  public:
    //! if this function returns true, no serialization is necessary
    virtual bool IsDefaultValue(ParamArchive& /*ar*/) const { return false; }
    //! serialize all values into current class
    virtual LSError Serialize(ParamArchive& ar) = 0;
    //! if IsDefaultValue returned true during Save, no class is saved
    //! and LoadDefaultValues is used instead of Serialize during Load
    virtual void LoadDefaultValues(ParamArchive& /*ar*/) {}
};
