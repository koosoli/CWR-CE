#pragma once

#include <Evaluator/express.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/Network/NetworkObject.hpp>

namespace Poseidon
{
using ResolveNetworkObjectFn = NetworkObject* (*)(void* context, const NetworkId& id);

enum class RemoteExecTargetKind : int
{
    Scalar = 0,
    Object = 1,
    Group = 2,
    Array = 3,
};

struct RemoteExecTargetSelector
{
    RemoteExecTargetKind kind = RemoteExecTargetKind::Scalar;
    int scalar = 0;
    NetworkId id = NetworkId::Null();
    AutoArray<RemoteExecTargetSelector> items;
};

bool SerializeScriptValue(QOStream& out, GameValuePar value);
bool EncodeScriptValue(AutoArray<char>& out, GameValuePar value);

GameValue DeserializeScriptValue(QIStream& in, ResolveNetworkObjectFn resolveObject, void* resolveContext,
                                 int depth = 0);
GameValue DecodeScriptValue(const AutoArray<char>& bytes, ResolveNetworkObjectFn resolveObject, void* resolveContext);

bool BuildRemoteExecTargetSelector(RemoteExecTargetSelector& out, GameValuePar value);
bool EncodeRemoteExecTargetSelector(AutoArray<char>& out, const RemoteExecTargetSelector& selector);
bool DecodeRemoteExecTargetSelector(RemoteExecTargetSelector& out, const AutoArray<char>& bytes);
} // namespace Poseidon
