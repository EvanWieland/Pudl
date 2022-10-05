// https://github.com/quantumsheep/demo-antlr4-llvm/blob/0bc8d6b04283f07869061f14f452de86a7c2dd4e/src/CLIManager.hpp

#pragma once

#include <algorithm>
#include <string>
#include <vector>

class CLIManager {
public:
    std::string program;
    std::vector<std::string> args;

    CLIManager(int argc, char **argv) : program(argv[0]) {
        for (int i = 1; i < argc; i++) {
            args.push_back(argv[i]);
        }
    }

    std::vector<std::string>::const_iterator find(const std::string &arg) const {
        return std::find(args.begin(), args.end(), arg);
    }

    bool hasOption(const std::string &option) const {
        return this->find(option) != args.end();
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
