#pragma once

#include <memory>

#include "types.hpp"


namespace memlib {
    class Memory;

    class Context final {

        std::unique_ptr<IBackend> pImpl;

        explicit Context(std::unique_ptr<IBackend> impl) : pImpl(std::move(impl)) {}
    public:

        Context() = delete;

        Context(Context&&) noexcept = default;
        Context& operator=(Context&&) noexcept = default;

        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;

        ~Context() = default;

        inline Result<void> read_bytes(memlib::address_t addr, memlib::buffer buffer, memlib::size_t size) {
            return pImpl ? pImpl->read_bytes(addr, buffer, size) :
                std::unexpected(Error(memlib::ErrorCode::NotInitialized));
        }
        inline Result<void> write_bytes(memlib::address_t addr, memlib::buffer buffer, memlib::size_t size) {
            return pImpl ? pImpl->write_bytes(addr, buffer, size) :
                std::unexpected(Error(memlib::ErrorCode::NotInitialized));
        }

        inline Result<memlib::u32> get_pid() const {
            return pImpl ? pImpl->get_pid() :
                std::unexpected(Error(memlib::ErrorCode::NotInitialized));
        }
        inline bool is_attached() const {
            return pImpl ? pImpl->is_attached() : false;
        }
        inline std::string get_name() const {
            return pImpl ? pImpl->get_name() : "LOSTED";
        }

        static Result<Context> internal();
    };
}