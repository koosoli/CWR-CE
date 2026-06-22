#include "PendingConnect.hpp"

namespace Poseidon
{

PendingConnect& GPendingConnect()
{
    static PendingConnect instance;
    return instance;
}

} // namespace Poseidon
