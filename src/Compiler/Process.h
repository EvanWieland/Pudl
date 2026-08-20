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
     * @return the child's exit code, or -1 if it could not be started.
     */
    static int Run(const std::vector<std::string> &args);
};
