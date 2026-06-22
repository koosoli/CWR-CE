#ifdef _MSC_VER
#pragma once
#endif

#ifndef _INIT_LIBRARY_ELEMENT_HPP
#define _INIT_LIBRARY_ELEMENT_HPP


namespace Poseidon
{
void InitParamFileStringtable();
void InitParamFileEvaluator();

inline void InitLibraryElement()
{
    InitParamFileStringtable();
    InitParamFileEvaluator();
}

#endif
} // namespace Poseidon

using Poseidon::InitLibraryElement;
using Poseidon::InitParamFileStringtable;
using Poseidon::InitParamFileEvaluator;
