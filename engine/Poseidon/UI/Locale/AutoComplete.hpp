#pragma once

#include <Poseidon/Foundation/Strings/RString.hpp>



namespace Poseidon
{
class IAutoComplete: public RefCount
{
	public:
	
	virtual RString Guess
	(
		RString text, int caret, bool &certain, RString &beg
	) = 0;
	virtual void WordDone(RString word) = 0;
	virtual void AfterChar(RString text, int caret) = 0;
};

IAutoComplete *CreateAutoComplete(const char *type);

} // namespace Poseidon
