#pragma once

#include <string>
#include <vector>

/**
 * Runs an external program directly from its argv, with no shell involved
 * at any point. Unlike std::system(), which hands a single string to
 * /bin/sh (POSIX) or cmd.exe (Windows) for interpretation, every argument
 * here reaches the child process exactly as given -- shell metacharacters
 * in a filename or flag value (e.g. from a CLI argument) cannot be used to
 * inject additional commands.
 *
 * The platform-specific implementation (and, on Windows, <windows.h>) lives
 * entirely in Process.cpp -- <windows.h>'s BOOL/FLOAT typedefs collide with
 * Pudl's own (unscoped) TokenType::BOOL/FLOAT enumerators, so it must never
 * be visible from a header that Token.h could end up included alongside in
 * the same translation unit (as happens via Codegen.h + Parser.h).
 */
class Process {
public:
    /**
     * @param args argv for the child process; args[0] is the program to run
     *             (searched on PATH if it isn't a path itself).
     * @param silent if true, the child's stdout/stderr are redirected to
     *               the null device instead of inherited -- for probing
     *               whether a program can be launched at all (see Detect())
     *               without leaking its actual output into ours.
     * @return the child's exit code, or -1 if it could not be started.
     */
    static int Run(const std::vector<std::string> &args, bool silent = false);

    /**
     * Finds the first name in `candidates` that can actually be launched
     * (probed via Run({name, probeArg}) != -1). Useful for tools LLVM
     * distributes under a version-suffixed name (lli-18, clang++-18, ...)
     * with no reliable "unversioned" alias -- hardcoding one specific
     * version breaks the moment the installed LLVM's version differs.
     *
     * @return the first working candidate, or candidates[0] if none could
     *         be launched (so callers still get a plausible name to report
     *         in their own error output rather than an empty string).
     */
    static std::string Detect(const std::vector<std::string> &candidates, const std::string &probeArg = "--version");

    /**
     * Returns a filename unlikely to collide with any other Pudl process
     * (or any other call to this function within the same process):
     * "<prefix>_<pid>_<n><extension>" in the current working directory.
     * Used for scratch files (the compiler's intermediate object file, the
     * linker's throwaway main()-wrapper source, ...) that are created and
     * removed within a single call -- a fixed name like "temp.o" would
     * collide if two `pudl` invocations ran concurrently in the same
     * directory.
     */
    static std::string UniqueTempPath(const std::string &prefix, const std::string &extension);
};
