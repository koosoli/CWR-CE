#include <Poseidon/Dev/Diag/FrameProfiler.hpp>

namespace Poseidon
{
namespace Dev
{

FrameProfiler& GFrameProfiler()
{
    static FrameProfiler instance;
    return instance;
}

} // namespace Dev
} // namespace Poseidon
