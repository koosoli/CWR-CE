#include <Poseidon/IO/Serialization/SerializeBinExt.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>

namespace Poseidon
{
void operator<<(SerializeBinStream& s, Vector3& data)
{
    if (s.IsLoading())
    {
        Vector3P t;
        s.TransferBinary(&t, sizeof(t));
        data = Vector3(t.X(), t.Y(), t.Z());
    }
    else
    {
        Vector3P t(data.X(), data.Y(), data.Z());
        s.TransferBinary(&t, sizeof(t));
    }
}

void operator<<(SerializeBinStream& s, Matrix4& data)
{
    if (s.IsLoading())
    {
        Matrix4P t = M4IdentityP;
        s.TransferBinary(&t, sizeof(t));
        data = ConvertToM(t);
    }
    else
    {
        Matrix4P t = ConvertToP(data);
        s.TransferBinary(&t, sizeof(t));
    }
}

void operator<<(SerializeBinStream& s, Matrix3& data)
{
    if (s.IsLoading())
    {
        Matrix3P t = M3IdentityP;
        s.TransferBinary(&t, sizeof(t));
        data = ConvertToM(t);
    }
    else
    {
        Matrix3P t = ConvertToP(data);
        s.TransferBinary(&t, sizeof(t));
    }
}

} // namespace Poseidon
