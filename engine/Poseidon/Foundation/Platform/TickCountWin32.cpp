#include <Poseidon/Foundation/Common/Win.h>

#include <mmsystem.h>
#include <Poseidon/Foundation/Platform/AppFrameExt.hpp>

namespace Poseidon::Foundation
{
DWORD CWRFrameFunctions::TickCount()
{
    static DWORD offset = timeGetTime();
    return timeGetTime() - offset;
}

} // namespace Poseidon::Foundation
