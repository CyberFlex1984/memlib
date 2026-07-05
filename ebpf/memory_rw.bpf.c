#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include "common.h"

char LICENSE[] SEC("license") = "GPL";

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, uint32_t);
    __type(value, struct RWArgs);
} rw_args SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, uint32_t);
    __type(value, unsigned char[256]);
} write_data SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 16);
} result_ringbuf SEC(".maps");

SEC("raw_tp/sys_enter")
int trigger_rw(struct bpf_raw_tracepoint_args* ctx)
{
    uint32_t zero = 0;

    uint64_t pid_tgid = bpf_get_current_pid_tgid();
    uint32_t current_pid = pid_tgid >> 32;

    struct RWArgs* args = bpf_map_lookup_elem(&rw_args, &zero);
    if (!args) return 0;

    if (current_pid != args->target_pid) return 0;

    uint64_t size = args->size;
    if (size > 256) size = 256;

    struct Result* res = bpf_ringbuf_reserve(&result_ringbuf, 256, 0);
    if (!res) return 0;

    if (args->cmd == CMD_READ) {
        if (bpf_probe_read_user(res->data, 256, (void*)args->addr) != 0) {
            bpf_ringbuf_discard(res, 0);
            return 0;
        }
    } else if (args->cmd == CMD_WRITE) {
        unsigned char* buf = bpf_map_lookup_elem(&write_data, &zero);
        if (!buf) {
            bpf_ringbuf_discard(res, 0);
            return 0;
        }

        if (size == 0) {
            bpf_ringbuf_discard(res, 0);
            return 0;
        }

        if (bpf_probe_write_user((void*)args->addr, buf, size) != 0) {
            bpf_ringbuf_discard(res, 0);
            return 0;
        }
        res->data[0] = 'O';
        res->data[1] = 'K';
    } else {
        bpf_ringbuf_discard(res, 0);
        return 0;
    }

    bpf_ringbuf_submit(res, 0);
    return 0;
}