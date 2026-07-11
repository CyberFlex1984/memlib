#if defined(__linux__) && defined(MEMLIB_EBPF)


#include "backend/scan_utils.hpp"
#include "memlib/types.hpp"

#include <atomic>
#include <bpf/libbpf.h>
#include <linux/bpf.h>

#include "ebpf_backend.hpp"
#include "../../ebpf/common.h"

#include "external_backend_linux.hpp"

#include <algorithm>
#include <unistd.h>
#include <cstring>
#include <chrono>

#include <map>
#include <string>

namespace memlib {
    static int on_event(void* ctx, void* data, size_t size) {
        auto* backend = (EBPFBackend*)ctx;
        if(size > 256) return 0;

        std::memcpy(backend->result_data, data, size);
        backend->result_size = size;

        backend->result_ready.store(true, std::memory_order_release);

        return 0;
    }

    Result<std::unique_ptr<EBPFBackend>> EBPFBackend::create(const std::string& name) {
        auto pid = find_pid_by_name(name);
        if (!pid) {
            return std::unexpected(pid.error());
        }
        return EBPFBackend::create(pid.value());
    }

    Result<std::unique_ptr<EBPFBackend>> EBPFBackend::create(memlib::u32 pid) {
        auto backend = std::make_unique<EBPFBackend>();

        std::string obj_path = "./memory_rw.bpf.o";

        auto obj = bpf_object__open_file(obj_path.c_str(), nullptr);
        if(!obj){
            return std::unexpected(Error(ErrorCode::BackendError, "Failed to open eBPF object"));
        }

        if(bpf_object__load(obj) != 0) {
            bpf_object__close(obj);
            return std::unexpected(Error(ErrorCode::BackendError, "Failed to load eBPF program"));
        }

        auto prog = bpf_object__find_program_by_name(obj, "trigger_rw");
        if(!prog){
            bpf_object__close(obj);
            return std::unexpected(Error(ErrorCode::BackendError, "Failed to load eBPF program"));
        }


        struct bpf_map* args = nullptr;
        struct bpf_map* write_data = nullptr;

        struct bpf_map* map;
        bpf_object__for_each_map(map, obj) {
            const char* name = bpf_map__name(map);
            if(strcmp(name, "rw_args") == 0) {
                args = map;
            }
            else if(strcmp(name, "write_data") == 0) {
                write_data = map;
            }
        }

        if(!args || !write_data) {
            bpf_object__close(obj);
            return std::unexpected(Error(ErrorCode::BackendError, "Failed to find BPF maps"));
        }

        struct bpf_map* rb_map = bpf_object__find_map_by_name(obj, "result_ringbuf");
        if(!rb_map){
            bpf_object__close(obj);
            return std::unexpected(Error(ErrorCode::BackendError, "Failed to find ring buffer map"));
        }

        auto rb = ring_buffer__new(bpf_map__fd(rb_map), on_event, backend.get(), nullptr);
        if(!rb) {
            bpf_object__close(obj);
            return std::unexpected(Error(ErrorCode::BackendError, "Failed to create ring buffer"));
        }

        auto link = bpf_program__attach_raw_tracepoint(prog, "sys_enter");
        if(!link){
            ring_buffer__free(rb);
            bpf_object__close(obj);

            return std::unexpected(Error(ErrorCode::BackendError, "Failed to attach eBPF program"));
        }

        backend->obj = obj;
        backend->prog = prog;
        backend->link = link;
        backend->rb = rb;
        backend->args = args;
        backend->write_data = write_data;
        backend->pid = pid;
        backend->attached = true;

        return backend;
    }
    EBPFBackend::~EBPFBackend() {
        if(link) bpf_link__destroy(link);
        if(rb) ring_buffer__free(rb);
        if(obj) bpf_object__close(obj);
    }

    bool EBPFBackend::is_attached() const {
        return attached;
    }
    Result<memlib::u32> EBPFBackend::get_pid() const {
        if(!attached) {
            return std::unexpected(Error(ErrorCode::NotAttached));
        }
        return pid;
    }
    std::string EBPFBackend::get_name() const {
        return "eBPF";
    }
    Result<void> EBPFBackend::read_chunk(address_t addr, buffer buffer, size_t size) {
        if(size > 256) {
            return std::unexpected(Error(ErrorCode::InvalidAddress, "read_chunk size must be <= 256"));
        }

        RWArgs args;
        args.cmd = CMD_READ;
        args.target_pid = pid;
        args.addr = addr;
        args.size = size;

        uint32_t key = 0;
        int ret = bpf_map__update_elem(this->args, &key, sizeof(key), &args, sizeof(args), BPF_ANY);
        if(ret != 0) {
            return std::unexpected(Error(ErrorCode::BackendError, "Failed to update rw_args map: " + std::to_string(ret)));
        }

        result_ready.store(false, std::memory_order_release);

        constexpr int TIMEOUT_MS = 100;
        auto start = std::chrono::steady_clock::now();

        while(!result_ready.load(std::memory_order_acquire)) {
            auto elapsed = std::chrono::steady_clock::now() - start;
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

            if(elapsed_ms > TIMEOUT_MS) {
                return std::unexpected(Error(ErrorCode::Timeout, "eBPF read chunk timeout"));
            }

            ret = ring_buffer__poll(rb, 10);
            if(ret < 0){
                return std::unexpected(Error(ErrorCode::BackendError,
                    "ring_buffer__poll failed"));
            }
        }

        memcpy(buffer, result_data, size);
        return {};
    }
    Result<void> EBPFBackend::write_chunk(address_t addr, buffer buffer, size_t size) {
        if(size > 256) {
            return std::unexpected(Error(ErrorCode::InvalidAddress, "write_chunk size must be <= 256"));
        }

        unsigned char old_data[256];
        auto read_result = read_chunk(addr, old_data, 256);
        if (!read_result) {
            return std::unexpected(read_result.error());
        }

        memcpy(old_data, buffer, size); // because we need to fix less bytes and don't corrupt memory

        uint32_t key = 0;
        int ret = bpf_map__update_elem(write_data, &key, sizeof(key), old_data, 256, BPF_ANY);
        if(ret != 0){
            return std::unexpected(Error(ErrorCode::BackendError, "Failed to update write map: " + std::to_string(ret)));
        }

        RWArgs args;
        args.cmd = CMD_WRITE;
        args.target_pid = pid;
        args.addr = addr;
        args.size = 256;

        ret = bpf_map__update_elem(this->args, &key, sizeof(key), &args, sizeof(args), BPF_ANY);
        if(ret != 0){
            return std::unexpected(Error(ErrorCode::BackendError, "Failed to update rw_args map: " + std::to_string(ret)));
        }

        result_ready.store(false, std::memory_order_release);

        constexpr int TIMEOUT_MS = 100;
        auto start = std::chrono::steady_clock::now();

        while(!result_ready.load(std::memory_order_acquire)){
            auto elapsed = std::chrono::steady_clock::now() - start;
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

            if(elapsed_ms > TIMEOUT_MS){
                return std::unexpected(Error(ErrorCode::Timeout, "eBPF write chunk timeout"));
            }

            ret = ring_buffer__poll(rb, 10);
            if(ret < 0){
                return std::unexpected(Error(ErrorCode::BackendError, "ring_buffer__poll failed"));
            }
        }

        if(result_size < 2 || std::memcmp(result_data, "OK", 2) != 0) {
            return std::unexpected(Error(ErrorCode::WriteFailed, "eBPF write chunk failed"));
        }

        return {};
    }

    Result<void> EBPFBackend::read_bytes(address_t addr, buffer buffer, size_t size) {
        if(!attached) {
            return std::unexpected(Error(ErrorCode::NotAttached));
        }

        if (size <= 256) {
            return read_chunk(addr, buffer, size);
        }

        size_t offset = 0;
        uint8_t* dst = (uint8_t*)buffer;

        while(offset < size) {
            size_t chunk_size = std::min<size_t>(256, size - offset);
            address_t chunk_addr = addr + offset;

            auto read_result = read_chunk(chunk_addr, dst + offset, chunk_size);
            if (!read_result) {
                return std::unexpected(read_result.error());
            }

            offset += chunk_size;
        }

        return {};
    }

    Result<void> EBPFBackend::write_bytes(address_t addr, buffer buffer, size_t size) {
        if(!attached) {
            return std::unexpected(Error(ErrorCode::NotAttached));
        }

        if(size <= 256){
            return write_chunk(addr, buffer, size);
        }

        size_t offset = 0;
        uint8_t* src = (uint8_t*)buffer;

        while(offset < size){
            size_t chunk_size = std::min<size_t>(256, size - offset);
            address_t chunk_addr = addr + offset;

            auto write_result = write_chunk(chunk_addr, src + offset, chunk_size);
            if (!write_result) {
                return std::unexpected(write_result.error());
            }

            offset += chunk_size;
        }

        return {};
    }

    Result<std::vector<address_t>> EBPFBackend::scan(const Pattern& pattern, address_t start, address_t end) {
        if(!attached){
            return std::unexpected(Error(ErrorCode::NotAttached));
        }
        return scan_region(*this, pattern, start, end);
    }
    Result<std::vector<ModuleInfo>> EBPFBackend::get_modules() {
        auto backend = ExternalLinuxBackend::create(pid);
        if (!backend) {
            return std::unexpected(backend.error());
        }
        return backend.value()->get_modules();
    }
    Result<ModuleInfo> EBPFBackend::get_module(const std::string& name) {
        auto backend = ExternalLinuxBackend::create(pid);
        if (!backend) {
            return std::unexpected(backend.error());
        }
        return backend.value()->get_module(name);
    }
}

#endif
