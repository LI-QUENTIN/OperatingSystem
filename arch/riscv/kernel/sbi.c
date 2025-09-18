#include "stdint.h"
#include "sbi.h"

// struct sbiret sbi_ecall(uint64_t eid, uint64_t fid,
//                         uint64_t arg0, uint64_t arg1, uint64_t arg2,
//                         uint64_t arg3, uint64_t arg4, uint64_t arg5) {
//     uint64_t error, value;
//     struct sbiret ret;
//     __asm__ volatile (
//     "mv a7, %[eid]\n"
// 	"mv a6, %[fid]\n"
// 	"mv a0, %[arg0]\n"
// 	"mv a1, %[arg1]\n"
// 	"mv a2, %[arg2]\n"
// 	"mv a3, %[arg3]\n"
// 	"mv a4, %[arg4]\n"
// 	"mv a5, %[arg5]\n"
// 	"ecall\n"
// 	"mv %[error], a0\n"
// 	"mv %[value], a1\n"
// 	: [error] "=r" (error), [value] "=r" (value)
// 	: [eid] "r" (eid), [fid] "r" (fid), [arg0] "r" (arg0), [arg1] "r" (arg1), 
// 	[arg2] "r" (arg2), [arg3] "r" (arg3), [arg4] "r" (arg4), [arg5] "r" (arg5)
// 	: "memory"
//     );
//     ret.error=error;
//     ret.value=value;
//     return ret;
// }


struct sbiret sbi_ecall(uint64_t eid, uint64_t fid,
                        uint64_t arg0, uint64_t arg1, uint64_t arg2,
                        uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0);
  	register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1);
  	register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2);
  	register uintptr_t a3 asm ("a3") = (uintptr_t)(arg3);
  	register uintptr_t a4 asm ("a4") = (uintptr_t)(arg4);
  	register uintptr_t a5 asm ("a5") = (uintptr_t)(arg5);
  	register uintptr_t a6 asm ("a6") = (uintptr_t)(fid);
  	register uintptr_t a7 asm ("a7") = (uintptr_t)(eid);
  	struct sbiret ret;

  asm volatile (
  	"ecall"
  	: "+r" (a0), "+r" (a1)
  	: "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
  	: "memory");
  ret.error = a0;
  ret.value = a1;
  return ret;
}


struct sbiret sbi_debug_console_write_byte(uint8_t byte) {
   return sbi_ecall(0x4442434e, 2, byte, 0, 0, 0, 0, 0);
}

struct sbiret sbi_system_reset(uint32_t reset_type, uint32_t reset_reason){
    return sbi_ecall(0x53525354, 0, reset_type, reset_reason, 0, 0, 0, 0);
}

struct sbiret sbi_set_timer(uint64_t stime_value) {
    return sbi_ecall(0x54494d45, 0, stime_value, 0, 0, 0, 0, 0);
}

struct sbiret sbi_debug_console_read(uint64_t num_bytes, uint64_t base_addr_lo, uint64_t base_addr_hi)
{
    struct sbiret result=sbi_ecall(0x4442434e,1,num_bytes,base_addr_lo,base_addr_hi,0,0,0);
    return result;
}