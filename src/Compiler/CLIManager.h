// https://github.com/quantumsheep/demo-antlr4-llvm/blob/0bc8d6b04283f07869061f14f452de86a7c2dd4e/src/CLIManager.hpp

#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

class CLIManager {
public:
    std::string program;
    std::vector<std::string> args;

    CLIManager(int argc, char **argv) : program(argv[0]) {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            // Split "--output=foo"/"-o=foo" into the two logical tokens
            // "--output"/"-o" and "foo" -- everything downstream
            // (find/hasOption/getOptionValue) already expects a flag and
            // its value as separate, adjacent entries (the space-separated
            // "-o foo" form), so this is the only place that needs to
            // know both syntaxes exist.
            std::string::size_type eq = arg.find('=');
            if (arg.size() > 1 && arg[0] == '-' && eq != std::string::npos) {
                args.push_back(arg.substr(0, eq));
                args.push_back(arg.substr(eq + 1));
            } else {
                args.push_back(arg);
            }
        }
    }

    std::vector<std::string>::const_iterator find(const std::string &arg) const {
        return std::find(args.begin(), args.end(), arg);
    }

    bool hasOption(const std::string &option) const {
        return this->find(option) != args.end();
    }

    /**
     * Prints a warning (not an error -- a typo'd flag shouldn't stop a
     * build) to stderr for every "-"-prefixed argument that isn't in
     * `knownOptions`. Best-effort: an option's own value can itself start
     * with "-" (e.g. an unusually-named linker) and would be flagged too;
     * accepted as a rare false positive in exchange for not needing to
     * duplicate main()'s value-vs-flag parsing logic here.
     */
    void warnUnknownOptions(const std::vector<std::string> &knownOptions) const {
        for (const std::string &arg: args) {
            if (arg.size() > 1 && arg[0] == '-'
                && std::find(knownOptions.begin(), knownOptions.end(), arg) == knownOptions.end()) {
                std::cerr << "Warning: unrecognized option '" << arg << "' (ignored)" << std::endl;
            }
        }
    }

    std::string getOptionValue(const std::string &option, const std::string &defaultValue = "") const {
        auto it = this->find(option);

        if (it == args.end())
            return defaultValue;

        it++;

        if (it == args.end())
            return defaultValue;

        return *it;
    }
};
