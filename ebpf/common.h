#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_READ  1
#define CMD_WRITE 2

struct RWArgs {
    uint32_t cmd;
    uint32_t target_pid;
    uint64_t addr;
    uint64_t size;
};

struct Result {
    uint8_t data[256];
};

#ifdef __cplusplus
}
#endif