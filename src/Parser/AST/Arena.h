#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

/**
 * Bump allocator for AST nodes.
 *
 * The AST used to be "new and never delete": every node lived until the
 * process exited, which happened to work only because nothing outlived
 * process exit anyway -- but it also meant Codegen.h::visit(FunctionDefNode)
 * couldn't safely keep a pointer to the node it was visiting (the visitor
 * pattern passed nodes *by value*, so `&aNode` pointed at a stack-local
 * copy that dangled the moment that visit() call returned). Nodes are now
 * arena-owned and the visitor takes nodes by reference (see ASTVisitor.h),
 * so `&aNode` is a pointer into arena memory that stays valid for the
 * arena's lifetime -- which Parser ties to one compilation (the Parser
 * instance, and its Arena member, live until main() returns).
 *
 * Unlike a "leak forever" bump allocator, this one still runs each
 * constructed object's destructor (in reverse construction order) when the
 * Arena itself is destroyed, so members like a node's std::string/
 * std::vector are properly freed instead of leaking their own heap
 * buffers -- the arena only changes *where* the fixed-size object storage
 * comes from, not whether destructors run.
 */
class Arena {
private:
    static constexpr std::size_t BlockSize = 64 * 1024;

    std::vector<std::unique_ptr<std::byte[]>> blocks;
    std::byte *current = nullptr;
    std::size_t remaining = 0;

    std::vector<std::pair<void *, void (*)(void *)>> destructors;

    void addBlock(std::size_t aMinSize) {
        std::size_t size = aMinSize > BlockSize ? aMinSize : BlockSize;
        auto block = std::make_unique<std::byte[]>(size);
        current = block.get();
        remaining = size;
        blocks.push_back(std::move(block));
    }

public:
    Arena() = default;

    Arena(const Arena &) = delete;

    Arena &operator=(const Arena &) = delete;

    ~Arena() {
        for (auto it = destructors.rbegin(); it != destructors.rend(); ++it) {
            it->second(it->first);
        }
    }

    template<typename T, typename... Args>
    T *construct(Args &&... aArgs) {
        void *ptr = current;
        std::size_t space = remaining;

        if (current == nullptr || std::align(alignof(T), sizeof(T), ptr, space) == nullptr) {
            addBlock(sizeof(T) + alignof(T));
            ptr = current;
            space = remaining;
            std::align(alignof(T), sizeof(T), ptr, space);
        }

        T *result = new(ptr) T(std::forward<Args>(aArgs)...);

        remaining = space - sizeof(T);
        current = static_cast<std::byte *>(ptr) + sizeof(T);

        if constexpr (!std::is_trivially_destructible_v<T>) {
            destructors.emplace_back(static_cast<void *>(result), [](void *aPtr) {
                static_cast<T *>(aPtr)->~T();
            });
        }

        return result;
    }
};
