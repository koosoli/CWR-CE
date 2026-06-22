#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Network/NetworkScriptValueCodec.hpp>

#include <string.h>

using namespace Poseidon;

namespace
{
GameValue RoundTrip(GameValuePar value)
{
    AutoArray<char> bytes;
    REQUIRE(EncodeScriptValue(bytes, value));
    return DecodeScriptValue(bytes, nullptr, nullptr);
}
} // namespace

TEST_CASE("NetworkScriptValueCodec round-trips scalar bool and string", "[network][remoteExec][codec]")
{
    GameValue scalar = RoundTrip(GameValue((GameScalarType)42.5f));
    REQUIRE(scalar.GetType() == GameScalar);
    REQUIRE((GameScalarType)scalar == Catch::Approx(42.5f));

    GameValue boolean = RoundTrip(GameValue((GameBoolType) true));
    REQUIRE(boolean.GetType() == GameBool);
    REQUIRE((GameBoolType)boolean);

    GameValue string = RoundTrip(GameValue((GameStringType)RString("payload")));
    REQUIRE(string.GetType() == GameString);
    REQUIRE((RString)(GameStringType)string == RString("payload"));
}

TEST_CASE("NetworkScriptValueCodec round-trips nested arrays", "[network][remoteExec][codec]")
{
    GameArrayType nested;
    nested.Add(GameValue((GameStringType)RString("inner")));
    nested.Add(GameValue((GameScalarType)7.0f));

    GameArrayType outer;
    outer.Add(GameValue((GameBoolType) false));
    outer.Add(GameValue(nested));

    GameValue decoded = RoundTrip(GameValue(outer));
    REQUIRE(decoded.GetType() == GameArray);
    const GameArrayType& decodedOuter = decoded.GetData()->GetArray();
    REQUIRE(decodedOuter.Size() == 2);
    REQUIRE(decodedOuter[0].GetType() == GameBool);
    REQUIRE_FALSE((GameBoolType)decodedOuter[0]);
    REQUIRE(decodedOuter[1].GetType() == GameArray);

    const GameArrayType& decodedNested = decodedOuter[1].GetData()->GetArray();
    REQUIRE(decodedNested.Size() == 2);
    REQUIRE((RString)(GameStringType)decodedNested[0] == RString("inner"));
    REQUIRE((GameScalarType)decodedNested[1] == Catch::Approx(7.0f));
}

TEST_CASE("NetworkScriptValueCodec rejects unsupported nil payloads", "[network][remoteExec][codec]")
{
    AutoArray<char> bytes;
    REQUIRE_FALSE(EncodeScriptValue(bytes, GameValue()));
}

TEST_CASE("NetworkScriptValueCodec rejects malformed payload bytes", "[network][remoteExec][codec]")
{
    AutoArray<char> bytes;
    bytes.Resize(sizeof(int));
    int type = GameString;
    memcpy(bytes.Data(), &type, sizeof(type));

    GameValue decoded = DecodeScriptValue(bytes, nullptr, nullptr);
    REQUIRE(decoded.GetNil());
}

TEST_CASE("RemoteExecTargetSelector codec round-trips nested selector arrays", "[network][remoteExec][codec]")
{
    RemoteExecTargetSelector scalar;
    scalar.kind = RemoteExecTargetKind::Scalar;
    scalar.scalar = -2;
    RemoteExecTargetSelector object;
    object.kind = RemoteExecTargetKind::Object;
    object.id = NetworkId(7, 42);
    RemoteExecTargetSelector root;
    root.kind = RemoteExecTargetKind::Array;
    root.items.Add(scalar);
    root.items.Add(object);

    AutoArray<char> bytes;
    REQUIRE(EncodeRemoteExecTargetSelector(bytes, root));

    RemoteExecTargetSelector decoded;
    REQUIRE(DecodeRemoteExecTargetSelector(decoded, bytes));
    REQUIRE(decoded.kind == RemoteExecTargetKind::Array);
    REQUIRE(decoded.items.Size() == 2);
    REQUIRE(decoded.items[0].kind == RemoteExecTargetKind::Scalar);
    REQUIRE(decoded.items[0].scalar == -2);
    REQUIRE(decoded.items[1].kind == RemoteExecTargetKind::Object);
    REQUIRE(decoded.items[1].id == NetworkId(7, 42));
}
