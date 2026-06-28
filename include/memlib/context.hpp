#pragma once

#include <memory>

#include "types.hpp"


namespace memlib {
    class Memory;

    class Context final {

        std::unique_ptr<IBackend> pImpl;

        explicit Context(std::unique_ptr<IBackend> impl) : pImpl(std::move(impl)) {}
    public:

        Context(Context&&) noexcept = default;
        Context& operator=(Context&&) noexcept = default;

        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;

        ~Context() = default;

        static Result<Context> internal();
    };
}