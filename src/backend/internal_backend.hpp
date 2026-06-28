#pragma once

#include <cstring>
#include <memlib/types.hpp>

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

        bool is_attached() const override {
            return true;
        }

        memlib::u32 get_pid() const override {
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