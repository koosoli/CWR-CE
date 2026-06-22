#include <Poseidon/Network/NetTransportPlayerValidation.hpp>
#include <string.h>
#include <Poseidon/Foundation/platform.hpp>

namespace Poseidon
{

ConnectResult ValidateNetTransportCreatePlayerRequest(const SessionPacket& session, const char* serverPassword,
                                                      const CreatePlayerPacket& request)
{
    if (request.actualVersion < session.requiredVersion || session.actualVersion < request.requiredVersion ||
        ((session.equalModRequired & 1) != 0 && stricmp(session.mod, request.mod) != 0) ||
        stricmp(session.versionTag, request.versionTag) != 0)
    {
        return CRVersion;
    }

    if (strncmp(request.password, serverPassword != nullptr ? serverPassword : "", LEN_PASSWORD_NAME) != 0)
    {
        return CRPassword;
    }

    return CROK;
}

} // namespace Poseidon
