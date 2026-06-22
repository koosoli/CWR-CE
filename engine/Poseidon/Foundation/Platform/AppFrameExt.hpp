#pragma once

#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <Poseidon/Core/Application.hpp>


namespace Poseidon::Foundation
{
class CWRFrameFunctions : public AppFrameFunctions
{
public:
	void ErrorMessage(const char *format, va_list argptr) override;
	void ErrorMessage(ErrorMessageLevel level, const char *format, va_list argptr) override;
	void WarningMessage(const char *format, va_list argptr) override;

	void ShowMessage(int timeMs, const char *msg, va_list argptr) override;
	DWORD TickCount() override;
};

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::CWRFrameFunctions;
