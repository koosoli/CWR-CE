#ifdef _MSC_VER
#pragma once
#endif

#ifndef _I_PREPROC_HPP
#define _I_PREPROC_HPP

#include <Poseidon/IO/Streams/QBStream.hpp>


namespace Poseidon
{
class PreprocessorFunctions
{
  public:
    PreprocessorFunctions() = default;
    virtual ~PreprocessorFunctions() = default;

    virtual bool Preprocess(QOStream& out, const char* name)
    {
        if (!QIFStreamB::FileExist(name))
        {
            return false;
        }

        QIFStreamB in;
        in.AutoOpen(name);
        out.write(in.act(), in.rest());
        return true;
    }
};

} // namespace Poseidon

#endif
