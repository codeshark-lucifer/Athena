#include "arch/riscv_asm.h"

extern "C" uint32_t riscv_alu_op(uint32_t x, uint32_t y)
{
    uint32_t result;
    __asm__ volatile(
        "add t0, %1, %2\n\t"
        "sub t1, %1, %2\n\t"
        "xor %0, t0, t1\n\t"
        : "=r"(result)
        : "r"(x), "r"(y)
        : "t0", "t1");
    return result;
}