#include "Process.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

#ifndef _WIN32

static int runPosix(const std::vector<std::string> &args, bool silent) {
    std::vector<char *> argv;
    argv.reserve(args.size() + 1);
    for (const auto &arg: args) {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr);

    posix_spawn_file_actions_t fileActions;
    posix_spawn_file_actions_t *fileActionsPtr = nullptr;
    if (silent) {
        posix_spawn_file_actions_init(&fileActions);
        posix_spawn_file_actions_addopen(&fileActions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
        posix_spawn_file_actions_addopen(&fileActions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
        fileActionsPtr = &fileActions;
    }

    pid_t pid;
    int rc = posix_spawnp(&pid, argv[0], fileActionsPtr, nullptr, argv.data(), environ);

    if (silent) {
        posix_spawn_file_actions_destroy(&fileActions);
    }

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

static int runWindows(const std::vector<std::string> &args, bool silent) {
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

    HANDLE nulHandle = INVALID_HANDLE_VALUE;
    if (silent) {
        SECURITY_ATTRIBUTES sa;
        ZeroMemory(&sa, sizeof(sa));
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;

        nulHandle = CreateFileA(
                "NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sa,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
        );

        if (nulHandle != INVALID_HANDLE_VALUE) {
            si.dwFlags |= STARTF_USESTDHANDLES;
            si.hStdOutput = nulHandle;
            si.hStdError = nulHandle;
            si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        }
    }

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
            /*bInheritHandles=*/ silent && nulHandle != INVALID_HANDLE_VALUE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi
    );

    if (nulHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(nulHandle);
    }

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

int Process::Run(const std::vector<std::string> &args, bool silent) {
    if (args.empty()) {
        return -1;
    }

#ifdef _WIN32
    return runWindows(args, silent);
#else
    return runPosix(args, silent);
#endif
}

std::string Process::Detect(const std::vector<std::string> &candidates, const std::string &probeArg) {
    for (const auto &candidate: candidates) {
        if (Run({candidate, probeArg}, /*silent=*/ true) != -1) {
            return candidate;
        }
    }
    return candidates.empty() ? "" : candidates[0];
}
