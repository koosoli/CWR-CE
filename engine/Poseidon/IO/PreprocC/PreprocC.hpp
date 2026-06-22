#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PREPROC_C_HPP
#define _PREPROC_C_HPP

#include <Poseidon/IO/PreprocC/IPreproc.hpp>

namespace Poseidon
{

class CPreprocessorFunctions final : public PreprocessorFunctions
{
  public:
    [[nodiscard]] bool Preprocess(QOStream& out, const char* name) override;
};

} // namespace Poseidon

using Poseidon::CPreprocessorFunctions;

#endif
