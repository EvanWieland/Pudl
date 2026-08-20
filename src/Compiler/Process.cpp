#include "Process.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

#ifndef _WIN32

static int runPosix(const std::vector<std::string> &args) {
    std::vector<char *> argv;
    argv.reserve(args.size() + 1);
    for (const auto &arg: args) {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid;
    int rc = posix_spawnp(&pid, argv[0], nullptr, nullptr, argv.data(), environ);
    if (rc != 0) {
        return -1;
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

#endif

#ifdef _WIN32

// Minimal Win32-argv-compatible quoting: wrap in quotes and escape embedded
// quotes. Sufficient for the plain paths/flags Pudl passes.
static std::string quoteArg(const std::string &arg) {
    std::string out = "\"";
    for (char c: arg) {
        if (c == '"') {
            out += "\\\"";
        } else {
            out += c;
        }
    }
    out += "\"";
    return out;
}

static int runWindows(const std::vector<std::string> &args) {
    std::string cmdLine;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) cmdLine += " ";
        cmdLine += quoteArg(args[i]);
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::vector<char> cmdLineBuf(cmdLine.begin(), cmdLine.end());
    cmdLineBuf.push_back('\0');

    // CreateProcessA is handed a mutable, pre-built command line (not a
    // shell string it re-parses/re-interprets) -- it just splits this back
    // into argv on the other side per Win32 quoting rules.
    BOOL ok = CreateProcessA(
            nullptr,
            cmdLineBuf.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi
    );

    if (!ok) {
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return static_cast<int>(exitCode);
}

#endif

int Process::Run(const std::vector<std::string> &args) {
    if (args.empty()) {
        return -1;
    }

#ifdef _WIN32
    return runWindows(args);
#else
    return runPosix(args);
#endif
}
