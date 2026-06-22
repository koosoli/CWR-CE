#include <Poseidon/World/Simulation/FrameInv.hpp>

namespace Poseidon
{
FrameWithInverse::FrameWithInverse(Matrix4Par trans, Matrix4Par invTrans)
{
    Frame::SetTransform(trans);
    _invTransform = invTrans;
}

} // namespace Poseidon
