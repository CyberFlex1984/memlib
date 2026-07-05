#pragma once

#include <memory>
#include "memlib/ibackend.hpp"
#include "types.hpp"

namespace memlib {

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

        inline Result<void> read_bytes(address_t addr, buffer buffer, size_t size) {
            return pImpl ? pImpl->read_bytes(addr, buffer, size) :
                std::unexpected(Error(ErrorCode::NotInitialized));
        }
        inline Result<void> write_bytes(address_t addr, buffer buffer, size_t size) {
            return pImpl ? pImpl->write_bytes(addr, buffer, size) :
                std::unexpected(Error(ErrorCode::NotInitialized));
        }

        inline Result<u32> get_pid() const {
            return pImpl ? pImpl->get_pid() :
                std::unexpected(Error(ErrorCode::NotInitialized));
        }
        inline bool is_attached() const {
            return pImpl ? pImpl->is_attached() : false;
        }
        inline std::string get_name() const {
            return pImpl ? pImpl->get_name() : "LOSTED";
        }
        inline auto get_modules() const {
            return pImpl ? pImpl->get_modules() : std::unexpected(Error(ErrorCode::NotInitialized));
        }
        inline auto get_module(const std::string& name) const {
            return pImpl ? pImpl->get_module(name) : std::unexpected(Error(ErrorCode::NotInitialized));
        }

        class Memory {
            Context& ctx;
            address_t addr;
        public:
            Memory(Context& ctx, address_t addr): ctx(ctx), addr(addr) {}

            Memory(const Memory&) = default;
        
            template<typename T>
            inline Result<T> read() const {
                T value;
                TRY(ctx.read_bytes(addr, &value, sizeof(T)));
                return value;
            }

            template<typename T>
            inline Result<void> write(const T& value) {
                return ctx.write_bytes(addr, (void*)&value, sizeof(T));
            }

            inline Result<byte_array> read_bytes(size_t size) const {
                byte_array buffer(size);
                TRY(ctx.read_bytes(addr, buffer.data(), size));
                return buffer;
            }

            inline Result<void> write_bytes(const byte_array& data) {
                return ctx.write_bytes(addr, (void*)data.data(), data.size());
            }

            inline Memory add(ptrdiff_t offset) const {
                return Memory(ctx, addr + offset);
            }

            inline Memory sub(ptrdiff_t offset) const {
                return Memory(ctx, addr - offset);
            }
            inline Memory& add_inplace(ptrdiff_t offset) {
                addr += offset;
                return *this;
            }
            inline Memory& sub_inplace(ptrdiff_t offset) {
                addr -= offset;
                return *this;
            }
            inline Result<Memory> deref() const {
                auto ptr = TRY(read<address_t>());
                return Memory(ctx, ptr);
            }
            inline address_t address() const {
                return addr;
            }
            inline Context& context() const {
                return ctx;
            }
        };
        
        inline Memory at(address_t address) {
            return Memory(*this, address);
        }

        static Result<Context> internal();

        static Result<Context> external(memlib::u32 pid);
        static Result<Context> external(const std::string& name);

        static Result<Context> ebpf(memlib::u32 pid);
        static Result<Context> ebpf(const std::string& name);
    };

    
}