#pragma once

#include <string>

namespace Poseidon::Foundation
{

/// Load a string value from persistent prefs. Returns defaultValue if not found.
std::string prefsGetString(const char* appName, const char* key, const char* defaultValue = "");

/// Save a string value to persistent prefs.
void prefsSetString(const char* appName, const char* key, const char* value);

} // namespace Poseidon::Foundation
