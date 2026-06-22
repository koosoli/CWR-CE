#pragma once

#include <signal.h>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace Poseidon::Dev
{

class DebugThreadWatch;

class Debugger
{
	bool _isDebugger;
	SRef<DebugThreadWatch> _watch;

	public:
	Debugger();
	~Debugger();

	bool IsDebugger() const {return _isDebugger;}
	void ForceLogging();
	void ProcessAlive();
	void NextAliveExpected( int timeout );

	bool CheckingAlivePaused();

	void PauseCheckingAlive();
	void ResumeCheckingAlive();

};

#if defined(__has_builtin)
#if __has_builtin(__builtin_debugtrap)
#define BREAK() {static bool disableBreak;if (!disableBreak) __builtin_debugtrap();}
#else
#define BREAK() {static bool disableBreak;if (!disableBreak) raise(SIGTRAP);}
#endif
#else
#define BREAK() {static bool disableBreak;if (!disableBreak) raise(SIGTRAP);}
#endif

extern Debugger GDebugger;

} // namespace Poseidon::Dev
