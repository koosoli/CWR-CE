#pragma once

namespace Poseidon
{

extern int gUserFpsCap;

void ProcessMessagesNoWait();
void RenderFrame(float deltaT, bool enableDraw);
bool AppIdle();

} // namespace Poseidon
