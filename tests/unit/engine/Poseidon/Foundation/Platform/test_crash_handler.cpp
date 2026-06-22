#include <catch2/catch_test_macros.hpp>

#ifndef _WIN32

#include <Poseidon/Foundation/Platform/CrashHandler.hpp>

#include <csignal>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

// A Linux crash must leave an offline-symbolizable trace, not a bare kernel "Segmentation
// fault". Method: fork a child, install the handler pointed at a temp dir, raise SIGSEGV. The
// child dies with SIGSEGV (the handler re-raises after writing the report). The parent then
// asserts the report exists and carries every marker an offline symbolizer needs — the
// signal, the build commit, a backtrace, the raw return addresses, and the module load bases
// from /proc/self/maps (return-addr minus load-base = the offset for llvm-symbolizer/addr2line
// against the matching release binary). Broken-state delta: without InstallCrashHandler the
// child SIGSEGVs with no file written at all.

TEST_CASE("crash handler writes a symbolizable report on a fatal signal", "[platform][crash]")
{
    char dirTmpl[] = "/tmp/cwr_crash_test_XXXXXX";
    REQUIRE(mkdtemp(dirTmpl) != nullptr);
    std::string dir = dirTmpl;

    pid_t pid = fork();
    REQUIRE(pid >= 0);
    if (pid == 0)
    {
        Poseidon::Foundation::InstallCrashHandler(dir.c_str());
        raise(SIGSEGV);
        _exit(0); // unreachable: the handler re-raises to the default disposition
    }

    int status = 0;
    REQUIRE(waitpid(pid, &status, 0) == pid);
    REQUIRE(WIFSIGNALED(status));
    REQUIRE(WTERMSIG(status) == SIGSEGV);

    std::string path = dir + "/crash_" + std::to_string(pid) + ".txt";
    std::ifstream f(path);
    REQUIRE(f.good());
    std::stringstream ss;
    ss << f.rdbuf();
    const std::string report = ss.str();

    REQUIRE(report.find("=== CRASH: SIGSEGV") != std::string::npos);
    REQUIRE(report.find("commit ") != std::string::npos);
    REQUIRE(report.find("backtrace:") != std::string::npos);
    REQUIRE(report.find("return addresses:") != std::string::npos);
    REQUIRE(report.find("/proc/self/maps:") != std::string::npos);

    remove(path.c_str());
    rmdir(dir.c_str());
}

#endif
