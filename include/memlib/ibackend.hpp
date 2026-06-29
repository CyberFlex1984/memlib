#pragma once

#include "memlib/pattern.hpp"
#include "memlib/types.hpp"

namespace memlib {
    class IBackend {
    public:
        virtual ~IBackend() = default;

        virtual Result<void> read_bytes(memlib::address_t addr, memlib::buffer buffer, memlib::size_t size) = 0;
        virtual Result<void> write_bytes(memlib::address_t addr, memlib::buffer buffer, memlib::size_t size) = 0;

        virtual Result<std::vector<address_t>> scan(const Pattern& pattern, address_t start, address_t end) = 0;

        virtual bool is_attached() const = 0;
        virtual Result<memlib::u32> get_pid() const = 0;
        virtual std::string get_name() const = 0;
    };
}