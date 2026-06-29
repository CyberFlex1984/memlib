#pragma once

#include "memlib/ibackend.hpp"
#include "memlib/pattern.hpp"
#include <cstring>
#include <iterator>
#include <memlib/types.hpp>
#include <ranges>
#include <vector>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
#endif


namespace memlib {
    class InternalBackend : public IBackend {
    public:
        ~InternalBackend() override {}

        Result<void> read_bytes(memlib::address_t addr, memlib::buffer buffer, memlib::size_t size) override {
            std::memcpy(buffer, reinterpret_cast<memlib::buffer>(addr), size);
            return {};
        }
        Result<void> write_bytes(memlib::address_t addr, memlib::buffer buffer, memlib::size_t size) override {
            std::memcpy(reinterpret_cast<memlib::buffer>(addr), buffer, size);
            return {};
        }

        Result<std::vector<address_t>> scan(const Pattern& pattern, address_t start, address_t end) override {
            const byte_t* data = reinterpret_cast<const byte_t*>(start);
            size_t size = end - start;
            size_t pattern_size = pattern.size();

            if(size < pattern_size) {
                return std::vector<address_t>{};
            }

            auto positions = std::views::iota(size_t{0}, size - pattern_size + 1)
                | std::views::filter([&](size_t offset){
                    return pattern.matches(data, offset);
                })
                | std::views::transform([&](size_t offset){
                    return start + offset;
                });
            
            std::vector<address_t> results{};
            std::ranges::copy(positions, std::back_inserter(results));

            return results;
        }

        bool is_attached() const override {
            return true;
        }

        Result<memlib::u32> get_pid() const override {
            #if defined(_WIN32)
                return GetCurrentProcessId();
            #elif defined(__linux__)
                return static_cast<memlib::u32>(getpid());
            #endif
        }
        std::string get_name() const override {
            return "Internal";
        }
    };
}