#include <Poseidon/Network/NetworkScriptValueCodec.hpp>

#include <Poseidon/AI/AI.hpp>
#include <Poseidon/Game/Commands/GameStateExt.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>

#include <string.h>

namespace Poseidon
{
namespace
{
constexpr int kMaxScriptValueStringBytes = 1024 * 1024;
constexpr int kMaxScriptValueArrayCount = 10000;
constexpr int kMaxScriptValueArrayDepth = 32;
constexpr int kMaxRemoteExecTargetArrayCount = 256;
constexpr int kMaxRemoteExecTargetArrayDepth = 16;

Object* GetValObject(GameValuePar oper)
{
    if (oper.GetType() == GameObject)
    {
        return static_cast<GameDataObject*>(oper.GetData())->GetObject();
    }
    return nullptr;
}

AIGroup* GetValGroup(GameValuePar oper)
{
    if (oper.GetType() == GameGroup)
    {
        return static_cast<GameDataGroup*>(oper.GetData())->GetGroup();
    }
    return nullptr;
}

bool SerializeRemoteExecTargetSelector(QOStream& out, const RemoteExecTargetSelector& selector, int depth)
{
    if (depth > kMaxRemoteExecTargetArrayDepth)
    {
        return false;
    }

    int kind = (int)selector.kind;
    out.write(&kind, sizeof(kind));
    if (selector.kind == RemoteExecTargetKind::Scalar)
    {
        out.write(&selector.scalar, sizeof(selector.scalar));
        return true;
    }
    if (selector.kind == RemoteExecTargetKind::Object || selector.kind == RemoteExecTargetKind::Group)
    {
        NetworkId id = selector.id;
        out.write(&id, sizeof(id));
        return true;
    }
    if (selector.kind == RemoteExecTargetKind::Array)
    {
        int count = selector.items.Size();
        if (count < 0 || count > kMaxRemoteExecTargetArrayCount)
        {
            return false;
        }
        out.write(&count, sizeof(count));
        for (int i = 0; i < count; ++i)
        {
            if (!SerializeRemoteExecTargetSelector(out, selector.items[i], depth + 1))
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool DeserializeRemoteExecTargetSelector(QIStream& in, RemoteExecTargetSelector& out, int depth)
{
    if (depth > kMaxRemoteExecTargetArrayDepth)
    {
        return false;
    }

    int kindValue;
    in.read(&kindValue, sizeof(kindValue));
    if (in.fail())
    {
        return false;
    }

    out = RemoteExecTargetSelector();
    out.kind = (RemoteExecTargetKind)kindValue;
    if (out.kind == RemoteExecTargetKind::Scalar)
    {
        in.read(&out.scalar, sizeof(out.scalar));
        return !in.fail();
    }
    if (out.kind == RemoteExecTargetKind::Object || out.kind == RemoteExecTargetKind::Group)
    {
        in.read(&out.id, sizeof(out.id));
        return !in.fail();
    }
    if (out.kind == RemoteExecTargetKind::Array)
    {
        int count;
        in.read(&count, sizeof(count));
        if (in.fail() || count < 0 || count > kMaxRemoteExecTargetArrayCount)
        {
            return false;
        }
        out.items.Resize(count);
        for (int i = 0; i < count; ++i)
        {
            if (!DeserializeRemoteExecTargetSelector(in, out.items[i], depth + 1))
            {
                return false;
            }
        }
        return true;
    }
    return false;
}
} // namespace

bool SerializeScriptValue(QOStream& out, GameValuePar value)
{
    int type = value.GetType();
    out.write(&type, sizeof(int));

    if (type == GameScalar)
    {
        GameScalarType scalar = value.GetData()->GetScalar();
        out.write(&scalar, sizeof(scalar));
        return true;
    }
    if (type == GameBool)
    {
        GameBoolType boolean = value.GetData()->GetBool();
        out.write(&boolean, sizeof(boolean));
        return true;
    }
    if (type == GameString)
    {
        GameStringType string = value.GetData()->GetString();
        int len = string.GetLength();
        out.write(&len, sizeof(int));
        out.write((const char*)string, len);
        return true;
    }
    if (type == GameObject)
    {
        Object* object = GetValObject(value);
        NetworkId id = object ? object->GetNetworkId() : NetworkId::Null();
        out.write(&id, sizeof(id));
        return true;
    }
    if (type == GameGroup)
    {
        AIGroup* group = GetValGroup(value);
        NetworkId id = group ? group->GetNetworkId() : NetworkId::Null();
        out.write(&id, sizeof(id));
        return true;
    }
    if (type == GameArray)
    {
        const GameArrayType& array = value.GetData()->GetArray();
        int count = array.Size();
        out.write(&count, sizeof(int));
        for (int i = 0; i < count; ++i)
        {
            if (!SerializeScriptValue(out, array[i]))
            {
                return false;
            }
        }
        return true;
    }
    if (type == GameNothing)
    {
        return true;
    }

    return false;
}

bool EncodeScriptValue(AutoArray<char>& out, GameValuePar value)
{
    QOStream stream;
    if (!SerializeScriptValue(stream, value))
    {
        return false;
    }

    int size = stream.pcount();
    out.Resize(size);
    if (size > 0)
    {
        memcpy(out.Data(), stream.str(), size);
    }
    return true;
}

GameValue DeserializeScriptValue(QIStream& in, ResolveNetworkObjectFn resolveObject, void* resolveContext, int depth)
{
    int type;
    in.read(&type, sizeof(int));
    if (in.fail())
    {
        return GameValue();
    }

    if (type == GameScalar)
    {
        GameScalarType value;
        in.read(&value, sizeof(value));
        return in.fail() ? GameValue() : GameValue(value);
    }
    if (type == GameBool)
    {
        GameBoolType value;
        in.read(&value, sizeof(value));
        return in.fail() ? GameValue() : GameValue(value);
    }
    if (type == GameString)
    {
        int len;
        in.read(&len, sizeof(int));
        if (in.fail() || len < 0 || len > kMaxScriptValueStringBytes)
        {
            return GameValue();
        }
        RString data("", len + 1);
        char* ptr = data.MutableData();
        in.read(ptr, len);
        if (in.fail())
        {
            return GameValue();
        }
        ptr[len] = 0;
        return GameValue((GameStringType)data);
    }
    if (type == GameObject)
    {
        NetworkId id;
        in.read(&id, sizeof(id));
        if (in.fail())
        {
            return GameValue();
        }
        NetworkObject* object = resolveObject ? resolveObject(resolveContext, id) : nullptr;
        return GameValueExt(dynamic_cast<Object*>(object));
    }
    if (type == GameGroup)
    {
        NetworkId id;
        in.read(&id, sizeof(id));
        if (in.fail())
        {
            return GameValue();
        }
        NetworkObject* object = resolveObject ? resolveObject(resolveContext, id) : nullptr;
        return GameValueExt(dynamic_cast<AIGroup*>(object));
    }
    if (type == GameArray)
    {
        if (depth >= kMaxScriptValueArrayDepth)
        {
            return GameValue();
        }
        int count;
        in.read(&count, sizeof(int));
        if (in.fail() || count < 0 || count > kMaxScriptValueArrayCount)
        {
            return GameValue();
        }
        GameArrayType array;
        array.Resize(count);
        for (int i = 0; i < count; ++i)
        {
            array[i] = DeserializeScriptValue(in, resolveObject, resolveContext, depth + 1);
            if (in.fail())
            {
                return GameValue();
            }
        }
        return GameValue(array);
    }
    if (type == GameNothing)
    {
        return GameValue();
    }

    return GameValue();
}

GameValue DecodeScriptValue(const AutoArray<char>& bytes, ResolveNetworkObjectFn resolveObject, void* resolveContext)
{
    QIStream stream;
    stream.init(bytes.Data(), bytes.Size());
    return DeserializeScriptValue(stream, resolveObject, resolveContext, 0);
}

bool BuildRemoteExecTargetSelector(RemoteExecTargetSelector& out, GameValuePar value)
{
    out = RemoteExecTargetSelector();
    if (value.GetType() == GameScalar)
    {
        out.kind = RemoteExecTargetKind::Scalar;
        out.scalar = (int)(GameScalarType)value;
        return true;
    }
    if (value.GetType() == GameObject)
    {
        out.kind = RemoteExecTargetKind::Object;
        Object* object = GetValObject(value);
        out.id = object ? object->GetNetworkId() : NetworkId::Null();
        return !out.id.IsNull();
    }
    if (value.GetType() == GameGroup)
    {
        out.kind = RemoteExecTargetKind::Group;
        AIGroup* group = GetValGroup(value);
        out.id = group ? group->GetNetworkId() : NetworkId::Null();
        return !out.id.IsNull();
    }
    if (value.GetType() == GameArray)
    {
        const GameArrayType& array = value.GetData()->GetArray();
        if (array.Size() > kMaxRemoteExecTargetArrayCount)
        {
            return false;
        }
        out.kind = RemoteExecTargetKind::Array;
        out.items.Resize(array.Size());
        for (int i = 0; i < array.Size(); ++i)
        {
            if (!BuildRemoteExecTargetSelector(out.items[i], array[i]))
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool EncodeRemoteExecTargetSelector(AutoArray<char>& out, const RemoteExecTargetSelector& selector)
{
    QOStream stream;
    if (!SerializeRemoteExecTargetSelector(stream, selector, 0))
    {
        return false;
    }

    int size = stream.pcount();
    out.Resize(size);
    if (size > 0)
    {
        memcpy(out.Data(), stream.str(), size);
    }
    return true;
}

bool DecodeRemoteExecTargetSelector(RemoteExecTargetSelector& out, const AutoArray<char>& bytes)
{
    QIStream stream;
    stream.init(bytes.Data(), bytes.Size());
    return DeserializeRemoteExecTargetSelector(stream, out, 0);
}
} // namespace Poseidon
