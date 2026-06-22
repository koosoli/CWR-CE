#include <catch2/catch_test_macros.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/IO/PreprocC/PreprocC.hpp>
#include <Poseidon/Network/NetworkServerAuth.hpp> // the extracted engine predicate
#include <cstring>
#include <catch2/catch_message.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

// Security PoC — dedicated-server admin login without a password
// Fizzy CWR #126 / Neptune Project. Stock OFP servers granted server admin
// ("gameMaster") to a client with NO password when the config omitted
// `passwordAdmin`. The NCMTLogin handler now requires a configured, non-empty
// `passwordAdmin` matched exactly, via Poseidon::AdminLoginPasswordAccepted
// (NetworkServerAuth.hpp); these cases are the regression that keeps it closed.
//
// The real-world trigger needs no custom tooling: the stock client's built-in
// `#login <anything>` chat command sends NCMTLogin (NetworkClientActions.cpp
// :1010 "login"->CMDLogin, :1136-1141 -> NCMTLogin with the typed string).
//
// This PoC drives the REAL engine ParamFile parser (the same class the server
// uses to parse server.cfg) and runs that exact decision against the engine
// predicate — no socket, no transport, no Glob.

namespace
{
// Text-config parsing needs a preprocessor registered on ParamFile.
void EnsurePreproc()
{
    static CPreprocessorFunctions* fns = nullptr;
    if (!fns)
    {
        fns = new CPreprocessorFunctions();
        ParamFile::SetDefaultPreprocFunctions(fns);
    }
}

ParamFile ParseServerCfg(const char* cfg)
{
    EnsurePreproc();
    ParamFile pf;
    QIStream in(cfg, strlen(cfg));
    pf.Parse(in);
    return pf;
}

// The authorization spec NCMTLogin enforces after the N-SEC-01 hardening: admin is
// granted only when a non-empty passwordAdmin is configured and matched exactly; a
// missing or empty entry grants nothing.
bool AdminLoginGranted(ParamFile& serverCfg, const char* providedPassword)
{
    ParamEntry* entry = serverCfg.FindEntry("passwordAdmin");
    if (!entry)
    {
        return false; // no admin password configured -> no remote admin
    }
    RStringB realPassword = *entry;
    if (realPassword.GetLength() == 0)
    {
        return false; // empty configured password -> no remote admin
    }
    return strcmp(providedPassword, realPassword.Data()) == 0;
}
} // namespace

TEST_CASE("N-SEC-01: a server with no passwordAdmin grants admin to no one", "[security][poc][network][admin]")
{
    // A realistic minimal dedicated-server config that simply forgot/omitted
    // passwordAdmin (very common in the wild).
    const char* cfgNoAdminPw = "hostname = \"My OFP Server\";\n"
                               "maxPlayers = 32;\n"
                               "voteThreshold = 0.5;\n";

    ParamFile cfg = ParseServerCfg(cfgNoAdminPw);
    REQUIRE(cfg.FindEntry("passwordAdmin") == nullptr); // the precondition

    // Broken-state delta: this used to accept ANY password (attacker -> gameMaster).
    // The hardened engine predicate now rejects every login when no password is set.
    REQUIRE_FALSE(Poseidon::AdminLoginPasswordAccepted(cfg, ""));
    REQUIRE_FALSE(Poseidon::AdminLoginPasswordAccepted(cfg, "wrong"));
    REQUIRE_FALSE(Poseidon::AdminLoginPasswordAccepted(cfg, "literally anything"));
}

TEST_CASE("N-SEC-01: an empty passwordAdmin string grants admin to no one", "[security][poc][network][admin]")
{
    // passwordAdmin present but empty — used to grant on an empty password.
    const char* cfgEmptyPw = "hostname = \"Server\";\n"
                             "passwordAdmin = \"\";\n";

    ParamFile cfg = ParseServerCfg(cfgEmptyPw);
    REQUIRE(cfg.FindEntry("passwordAdmin") != nullptr);

    REQUIRE_FALSE(Poseidon::AdminLoginPasswordAccepted(cfg, "")); // empty no longer an open door
    REQUIRE_FALSE(Poseidon::AdminLoginPasswordAccepted(cfg, "x"));
}

TEST_CASE("Control: a configured passwordAdmin gates admin correctly", "[security][poc][network][admin]")
{
    const char* cfgWithPw = "hostname = \"Server\";\n"
                            "passwordAdmin = \"s3cret\";\n"
                            "maxPlayers = 32;\n";

    ParamFile cfg = ParseServerCfg(cfgWithPw);
    REQUIRE(cfg.FindEntry("passwordAdmin") != nullptr);

    REQUIRE_FALSE(AdminLoginGranted(cfg, ""));       // empty rejected
    REQUIRE_FALSE(AdminLoginGranted(cfg, "wrong"));  // wrong rejected
    REQUIRE_FALSE(AdminLoginGranted(cfg, "S3CRET")); // case-sensitive
    REQUIRE(AdminLoginGranted(cfg, "s3cret"));       // correct -> admin
}

// Spec check for AdminLoginPasswordAccepted() (NetworkServerAuth.hpp).
// AdminLoginGranted() above is the hardened authorization spec (post N-SEC-01);
// this asserts the engine predicate matches it across a matrix of configs and
// passwords. If the engine logic ever drifts from the spec, this fails.
TEST_CASE("Refactor: AdminLoginPasswordAccepted equals the hardened auth spec", "[security][refactor][network][admin]")
{
    struct Case
    {
        const char* cfg;
        const char* pw;
    };
    const Case cases[] = {
        {"hostname = \"s\";\n", ""},
        {"hostname = \"s\";\n", "x"},
        {"hostname = \"s\";\nmaxPlayers = 8;\n", "anything"},
        {"passwordAdmin = \"\";\n", ""},
        {"passwordAdmin = \"\";\n", "x"},
        {"passwordAdmin = \"s3cret\";\n", ""},
        {"passwordAdmin = \"s3cret\";\n", "wrong"},
        {"passwordAdmin = \"s3cret\";\n", "S3CRET"},
        {"passwordAdmin = \"s3cret\";\n", "s3cret"},
    };
    for (const auto& c : cases)
    {
        ParamFile cfg = ParseServerCfg(c.cfg);
        INFO("cfg=[" << c.cfg << "] pw=[" << c.pw << "]");
        REQUIRE(Poseidon::AdminLoginPasswordAccepted(cfg, c.pw) == AdminLoginGranted(cfg, c.pw));
    }
}
