#pragma once

#include "context.hpp"
#include "memlib/types.hpp"
#include <cstddef>

namespace memlib {
    class Memory{
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
        inline Result<T> write(const T& value) {
            return ctx.write_bytes(addr, &value, sizeof(T));
        }

        inline Result<memlib::byte_array> read_bytes(memlib::size_t size) const {
            memlib::byte_array buffer(size);
            TRY(ctx.read_bytes(addr, buffer.data(), size));
            return buffer;
        }

        inline Result<void> write_bytes(const memlib::byte_array& data) {
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
        inline memlib::address_t address() const {
            return addr;
        }
        inline Context& context() const {
            return ctx;
        }
    };
}