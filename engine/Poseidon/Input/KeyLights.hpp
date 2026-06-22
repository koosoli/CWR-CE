#pragma once

namespace Poseidon
{

class KeyLights
{
	bool _globalScrollLock;
	bool _globalCapsLock;
	bool _acquired;

public:
	static bool GetScrollLock();
	static bool GetCapsLock();
	void SetScrollLock(bool state);
	void SetCapsLock(bool state);

	void GetGlobalState();
	void RestoreGlobalState();
	KeyLights();
	~KeyLights();
};

extern KeyLights KeyState;

} // namespace Poseidon

