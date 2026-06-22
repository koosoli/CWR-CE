#pragma once

namespace Poseidon::Foundation
{

// Attach stdout/stderr to parent console if available (Windows GUI apps).
// In debug builds, allocates a new console if no parent is found.
// No-op on non-Windows platforms.
void attachParentConsole();

} // namespace Poseidon::Foundation

