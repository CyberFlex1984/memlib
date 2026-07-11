#pragma once
#if defined(__linux__) && defined(MEMLIB_EBPF)

#include <memlib/ibackend.hpp>
#include <memlib/types.hpp>

#include <memory>
#include <atomic>
#include <mutex>

#include <bpf/libbpf.h>
#include <linux/bpf.h>

namespace memlib {
    class EBPFBackend : public IBackend {
    public:
        struct bpf_object* obj = nullptr;
        struct bpf_program* prog = nullptr;
        struct bpf_link* link = nullptr;
        struct ring_buffer* rb = nullptr;

        struct bpf_map* args = nullptr;      
        struct bpf_map* write_data = nullptr; 

        uint32_t pid = 0;
        bool attached = false;

    
        uint8_t result_data[256];
        size_t result_size = 0;
        std::atomic<bool> result_ready{false};

        EBPFBackend() = default;

        static Result<std::unique_ptr<EBPFBackend>> create(const std::string& name);
        static Result<std::unique_ptr<EBPFBackend>> create(memlib::u32 pid);

        
        ~EBPFBackend() override;

        Result<void> read_bytes(address_t addr, buffer buffer, size_t size) override;
        Result<void> write_bytes(address_t addr, buffer buffer, size_t size) override;

        Result<std::vector<address_t>> scan(const Pattern& pattern, address_t start, address_t end) override;

        bool is_attached() const override;
        Result<uint32_t> get_pid() const override;
        std::string get_name() const override;

        Result<std::vector<ModuleInfo>> get_modules() override;
        Result<ModuleInfo> get_module(const std::string& name) override;

    private:
        Result<void> read_chunk(address_t addr, buffer buffer, size_t size);
        Result<void> write_chunk(address_t addr, buffer buffer, size_t size);
    };
}

#endif
