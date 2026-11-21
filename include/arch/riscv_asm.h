// ASM
#pragma once
#include <cstdint>

#ifdef __cplusplus
extern "C"
{
#endif
    uint32_t riscv_alu_op(uint32_t x, uint32_t y);
#ifdef __cplusplus
}
#endif

static inline uint32_t read_riscv_cycles()
{
    uint32_t cycles;
    __asm__ volatile("csrr %0, mcycle" : "=r"(cycles));
    return cycles;
}