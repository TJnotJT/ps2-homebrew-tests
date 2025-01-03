# 0 "main.c"
# 1 "/usr/local/ps2dev/ps2sdk/samples/draw/pcsx2_test//"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "main.c"
# 12 "main.c"
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/kernel.h" 1
# 20 "/usr/local/ps2dev/ps2sdk/ee/include/kernel.h"
# 1 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stddef.h" 1 3 4
# 145 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stddef.h" 3 4

# 145 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stddef.h" 3 4
typedef int ptrdiff_t;
# 214 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stddef.h" 3 4
typedef unsigned int size_t;
# 329 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stddef.h" 3 4
typedef int wchar_t;
# 425 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stddef.h" 3 4
typedef struct {
  long long __max_align_ll __attribute__((__aligned__(__alignof__(long long))));
  long double __max_align_ld __attribute__((__aligned__(__alignof__(long double))));
# 436 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stddef.h" 3 4
} max_align_t;
# 21 "/usr/local/ps2dev/ps2sdk/ee/include/kernel.h" 2
# 1 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stdarg.h" 1 3 4
# 40 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 103 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stdarg.h" 3 4
typedef __gnuc_va_list va_list;
# 22 "/usr/local/ps2dev/ps2sdk/ee/include/kernel.h" 2
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/sifdma.h" 1
# 19 "/usr/local/ps2dev/ps2sdk/ee/include/sifdma.h"
# 1 "/usr/local/ps2dev/ps2sdk/common/include/tamtypes.h" 1
# 23 "/usr/local/ps2dev/ps2sdk/common/include/tamtypes.h"

# 23 "/usr/local/ps2dev/ps2sdk/common/include/tamtypes.h"
typedef unsigned char u8;
typedef unsigned short u16;

typedef volatile u8 vu8;
typedef volatile u16 vu16;


typedef unsigned int u32;

typedef unsigned long long u64;



typedef unsigned int u128 __attribute__((mode(TI)));

typedef volatile u32 vu32;
typedef volatile u64 vu64;
typedef volatile u128 vu128 __attribute__((mode(TI)));
# 51 "/usr/local/ps2dev/ps2sdk/common/include/tamtypes.h"
typedef signed char s8;
typedef signed short s16;

typedef volatile s8 vs8;
typedef volatile s16 vs16;


typedef signed int s32;

typedef signed long long s64;



typedef signed int s128 __attribute__((mode(TI)));

typedef volatile s32 vs32;
typedef volatile s64 vs64;
typedef volatile s128 vs128 __attribute__((mode(TI)));
# 80 "/usr/local/ps2dev/ps2sdk/common/include/tamtypes.h"
typedef u32 uiptr;
typedef s32 siptr;

typedef volatile u32 vuiptr;
typedef volatile s32 vsiptr;


typedef union
{
    u128 qw;
    u8 b[16];
    u16 hw[8];
    u32 sw[4];
    u64 dw[2];
} qword_t;







static inline u8 _lb(u32 addr)
{
    return *(vu8 *)addr;
}
static inline u16 _lh(u32 addr) { return *(vu16 *)addr; }
static inline u32 _lw(u32 addr) { return *(vu32 *)addr; }

static inline void _sb(u8 val, u32 addr) { *(vu8 *)addr = val; }
static inline void _sh(u16 val, u32 addr) { *(vu16 *)addr = val; }
static inline void _sw(u32 val, u32 addr) { *(vu32 *)addr = val; }


static inline u64 _ld(u32 addr)
{
    return *(vu64 *)addr;
}
static inline u128 _lq(u32 addr) { return *(vu128 *)addr; }
static inline void _sd(u64 val, u32 addr) { *(vu64 *)addr = val; }
static inline void _sq(u128 val, u32 addr) { *(vu128 *)addr = val; }
# 20 "/usr/local/ps2dev/ps2sdk/ee/include/sifdma.h" 2
# 28 "/usr/local/ps2dev/ps2sdk/ee/include/sifdma.h"
enum _sif_regs {

    SIF_REG_MAINADDR = 1,

    SIF_REG_SUBADDR,

    SIF_REG_MSFLAG,

    SIF_REG_SMFLAG,


    SIF_SYSREG_SUBADDR = 0x80000000 | 0,
    SIF_SYSREG_MAINADDR,
    SIF_SYSREG_RPCINIT,
};
# 52 "/usr/local/ps2dev/ps2sdk/ee/include/sifdma.h"
typedef struct t_SifDmaTransfer
{
    void *src,
        *dest;
    int size;
    int attr;
} SifDmaTransfer_t;





extern u32 SifSetDma(SifDmaTransfer_t *sdd, s32 len);
extern s32 SifDmaStat(u32 id);
# 23 "/usr/local/ps2dev/ps2sdk/ee/include/kernel.h" 2
# 55 "/usr/local/ps2dev/ps2sdk/ee/include/kernel.h"
extern void *ChangeGP(void *gp);
extern void SetGP(void *gp);
extern void *GetGP(void);

extern void *_gp;
# 83 "/usr/local/ps2dev/ps2sdk/ee/include/kernel.h"
enum {
    INTC_GS,
    INTC_SBUS,
    INTC_VBLANK_S,
    INTC_VBLANK_E,
    INTC_VIF0,
    INTC_VIF1,
    INTC_VU0,
    INTC_VU1,
    INTC_IPU,
    INTC_TIM0,
    INTC_TIM1,
    INTC_TIM2,

    INTC_SFIFO = 13,
    INTC_VU0WD
};
# 115 "/usr/local/ps2dev/ps2sdk/ee/include/kernel.h"
enum {
    DMAC_VIF0,
    DMAC_VIF1,
    DMAC_GIF,
    DMAC_FROM_IPU,
    DMAC_TO_IPU,
    DMAC_SIF0,
    DMAC_SIF1,
    DMAC_SIF2,
    DMAC_FROM_SPR,
    DMAC_TO_SPR,

    DMAC_CIS = 13,
    DMAC_MEIS,
    DMAC_BEIS,
};
# 141 "/usr/local/ps2dev/ps2sdk/ee/include/kernel.h"
static inline void nopdelay(void)
{
    int i = 0xfffff;

    do {
        __asm__("nop\nnop\nnop\nnop\nnop\n");
    } while (i-- != -1);
}

static inline int ee_get_opmode(void)
{
    u32 status;

    __asm__ volatile(
        ".set\tpush\n\t"
        ".set\tnoreorder\n\t"
        "mfc0\t%0, $12\n\t"
        ".set\tpop\n\t"
        : "=r"(status));

    return ((status >> 3) & 3);
}

static inline int ee_set_opmode(u32 opmode)
{
    u32 status, mask;

    __asm__ volatile(
        ".set\tpush\n\t"
        ".set\tnoreorder\n\t"
        "mfc0\t%0, $12\n\t"
        "li\t%1, 0xffffffe7\n\t"
        "and\t%0, %1\n\t"
        "or\t%0, %2\n\t"
        "mtc0\t%0, $12\n\t"
        "sync.p\n\t"
        ".set\tpop\n\t"
        : "=r"(status), "=r"(mask)
        : "r"(opmode));

    return ((status >> 3) & 3);
}

static inline int ee_kmode_enter()
{
    u32 status, mask;

    __asm__ volatile(
        ".set\tpush\n\t"
        ".set\tnoreorder\n\t"
        "mfc0\t%0, $12\n\t"
        "li\t%1, 0xffffffe7\n\t"
        "and\t%0, %1\n\t"
        "mtc0\t%0, $12\n\t"
        "sync.p\n\t"
        ".set\tpop\n\t"
        : "=r"(status), "=r"(mask));

    return status;
}

static inline int ee_kmode_exit()
{
    int status;

    __asm__ volatile(
        ".set\tpush\n\t"
        ".set\tnoreorder\n\t"
        "mfc0\t%0, $12\n\t"
        "ori\t%0, 0x10\n\t"
        "mtc0\t%0, $12\n\t"
        "sync.p\n\t"
        ".set\tpop\n\t"
        : "=r"(status));

    return status;
}

typedef struct t_ee_sema
{
    int count,
        max_count,
        init_count,
        wait_threads;
    u32 attr,
        option;
} ee_sema_t;

typedef struct t_ee_thread
{
    int status;
    void *func;
    void *stack;
    int stack_size;
    void *gp_reg;
    int initial_priority;
    int current_priority;
    u32 attr;
    u32 option;

} ee_thread_t;
# 257 "/usr/local/ps2dev/ps2sdk/ee/include/kernel.h"
typedef struct t_ee_thread_status
{
    int status;
    void *func;
    void *stack;
    int stack_size;
    void *gp_reg;
    int initial_priority;
    int current_priority;
    u32 attr;
    u32 option;
    u32 waitType;
    u32 waitId;
    u32 wakeupCount;
} ee_thread_status_t;


enum CPU_CONFIG {
    CPU_CONFIG_ENABLE_DIE = 0,
    CPU_CONFIG_ENABLE_ICE,
    CPU_CONFIG_ENABLE_DCE,
    CPU_CONFIG_DISBLE_DIE,
    CPU_CONFIG_DISBLE_ICE,
    CPU_CONFIG_DISBLE_DCE
};






enum {
    COP0_INDEX,
    COP0_RANDOM,
    COP0_ENTRYLO0,
    COP0_ENTRYLO1,
    COP0_CONTEXT,
    COP0_PAGEMASK,
    COP0_WIRED,

    COP0_BADVADDR = 8,
    COP0_COUNT,
    COP0_ENTRYHI,
    COP0_COMPARE,
    COP0_STATUS,
    COP0_CAUSE,
    COP0_EPC,
    COP0_PRID,
    COP0_CONFIG,

    COP0_BADPADDR = 23,
    COP0_DEBUG,
    COP0_PERF,

    COP0_TAGLO = 28,
    COP0_TAGHI,
    COP0_ERROREPC,
};






extern void _InitSys(void);

extern void TerminateLibrary(void);


extern int InitThread(void);

extern s32 iWakeupThread(s32 thread_id);
extern s32 iRotateThreadReadyQueue(s32 priority);
extern s32 iSuspendThread(s32 thread_id);


extern void InitTLBFunctions(void);

extern void InitTLB(void);
extern void Exit(s32 exit_code) __attribute__((noreturn));
extern s32 ExecPS2(void *entry, void *gp, int num_args, char *args[]);
extern void LoadExecPS2(const char *filename, s32 num_args, char *args[]) __attribute__((noreturn));
extern void ExecOSD(int num_args, char *args[]) __attribute__((noreturn));


extern void InitAlarm(void);


extern void InitExecPS2(void);
extern void InitOsd(void);

extern int PatchIsNeeded(void);


extern void InitDebug(void);


extern int DIntr(void);
extern int EIntr(void);

extern int EnableIntc(int intc);
extern int DisableIntc(int intc);
extern int EnableDmac(int dmac);
extern int DisableDmac(int dmac);

extern int iEnableIntc(int intc);
extern int iDisableIntc(int intc);
extern int iEnableDmac(int dmac);
extern int iDisableDmac(int dmac);

extern void SyncDCache(void *start, void *end);
extern void iSyncDCache(void *start, void *end);
extern void InvalidDCache(void *start, void *end);
extern void iInvalidDCache(void *start, void *end);


extern void ResetEE(u32 init_bitfield);
extern void SetGsCrt(s16 interlace, s16 pal_ntsc, s16 field);
extern void KExit(s32 exit_code) __attribute__((noreturn));
extern void _LoadExecPS2(const char *filename, s32 num_args, char *args[]) __attribute__((noreturn));
extern s32 _ExecPS2(void *entry, void *gp, int num_args, char *args[]);
extern void RFU009(u32 arg0, u32 arg1);
extern s32 AddSbusIntcHandler(s32 cause, void (*handler)(int call));
extern s32 RemoveSbusIntcHandler(s32 cause);
extern s32 Interrupt2Iop(s32 cause);
extern void SetVTLBRefillHandler(s32 handler_num, void *handler_func);
extern void SetVCommonHandler(s32 handler_num, void *handler_func);
extern void SetVInterruptHandler(s32 handler_num, void *handler_func);
extern s32 AddIntcHandler(s32 cause, s32 (*handler_func)(s32 cause), s32 next);
extern s32 AddIntcHandler2(s32 cause, s32 (*handler_func)(s32 cause, void *arg, void *addr), s32 next, void *arg);
extern s32 RemoveIntcHandler(s32 cause, s32 handler_id);
extern s32 AddDmacHandler(s32 channel, s32 (*handler)(s32 channel), s32 next);
extern s32 AddDmacHandler2(s32 channel, s32 (*handler)(s32 channel, void *arg, void *addr), s32 next, void *arg);
extern s32 RemoveDmacHandler(s32 channel, s32 handler_id);
extern s32 _EnableIntc(s32 cause);
extern s32 _DisableIntc(s32 cause);
extern s32 _EnableDmac(s32 channel);
extern s32 _DisableDmac(s32 channel);


extern s32 SetAlarm(u16 time, void (*callback)(s32 alarm_id, u16 time, void *common), void *common);
extern s32 _SetAlarm(u16 time, void (*callback)(s32 alarm_id, u16 time, void *common), void *common);
extern s32 ReleaseAlarm(s32 alarm_id);
extern s32 _ReleaseAlarm(s32 alarm_id);

extern s32 _iEnableIntc(s32 cause);
extern s32 _iDisableIntc(s32 cause);
extern s32 _iEnableDmac(s32 channel);
extern s32 _iDisableDmac(s32 channel);

extern s32 iSetAlarm(u16 time, void (*callback)(s32 alarm_id, u16 time, void *common), void *common);
extern s32 _iSetAlarm(u16 time, void (*callback)(s32 alarm_id, u16 time, void *common), void *common);
extern s32 iReleaseAlarm(s32 alarm_id);
extern s32 _iReleaseAlarm(s32 alarm_id);

extern s32 CreateThread(ee_thread_t *thread);
extern s32 DeleteThread(s32 thread_id);
extern s32 StartThread(s32 thread_id, void *args);
extern void ExitThread(void);
extern void ExitDeleteThread(void);
extern s32 TerminateThread(s32 thread_id);
extern s32 iTerminateThread(s32 thread_id);


extern s32 ChangeThreadPriority(s32 thread_id, s32 priority);
extern s32 iChangeThreadPriority(s32 thread_id, s32 priority);
extern s32 RotateThreadReadyQueue(s32 priority);
extern s32 _iRotateThreadReadyQueue(s32 priority);
extern s32 ReleaseWaitThread(s32 thread_id);
extern s32 iReleaseWaitThread(s32 thread_id);
extern s32 GetThreadId(void);
extern s32 _iGetThreadId(void);
extern s32 ReferThreadStatus(s32 thread_id, ee_thread_status_t *info);
extern s32 iReferThreadStatus(s32 thread_id, ee_thread_status_t *info);
extern s32 SleepThread(void);
extern s32 WakeupThread(s32 thread_id);
extern s32 _iWakeupThread(s32 thread_id);
extern s32 CancelWakeupThread(s32 thread_id);
extern s32 iCancelWakeupThread(s32 thread_id);
extern s32 SuspendThread(s32 thread_id);
extern s32 _iSuspendThread(s32 thread_id);
extern s32 ResumeThread(s32 thread_id);
extern s32 iResumeThread(s32 thread_id);

extern u8 RFU059(void);

extern void *SetupThread(void *gp, void *stack, s32 stack_size, void *args, void *root_func);
extern void SetupHeap(void *heap_start, s32 heap_size);
extern void *EndOfHeap(void);

extern s32 CreateSema(ee_sema_t *sema);
extern s32 DeleteSema(s32 sema_id);
extern s32 SignalSema(s32 sema_id);
extern s32 iSignalSema(s32 sema_id);
extern s32 WaitSema(s32 sema_id);
extern s32 PollSema(s32 sema_id);
extern s32 iPollSema(s32 sema_id);
extern s32 ReferSemaStatus(s32 sema_id, ee_sema_t *sema);
extern s32 iReferSemaStatus(s32 sema_id, ee_sema_t *sema);
extern s32 iDeleteSema(s32 sema_id);
extern void SetOsdConfigParam(void *addr);
extern void GetOsdConfigParam(void *addr);
extern void GetGsHParam(void *addr1, void *addr2, void *addr3);
extern s32 GetGsVParam(void);
extern void SetGsHParam(void *addr1, void *addr2, void *addr3, void *addr4);
extern void SetGsVParam(s32 arg1);


extern int PutTLBEntry(unsigned int PageMask, unsigned int EntryHi, unsigned int EntryLo0, unsigned int EntryLo1);
extern int iPutTLBEntry(unsigned int PageMask, unsigned int EntryHi, unsigned int EntryLo0, unsigned int EntryLo1);
extern int _SetTLBEntry(unsigned int index, unsigned int PageMask, unsigned int EntryHi, unsigned int EntryLo0, unsigned int EntryLo1);
extern int iSetTLBEntry(unsigned int index, unsigned int PageMask, unsigned int EntryHi, unsigned int EntryLo0, unsigned int EntryLo1);
extern int GetTLBEntry(unsigned int index, unsigned int *PageMask, unsigned int *EntryHi, unsigned int *EntryLo0, unsigned int *EntryLo1);
extern int iGetTLBEntry(unsigned int index, unsigned int *PageMask, unsigned int *EntryHi, unsigned int *EntryLo0, unsigned int *EntryLo1);
extern int ProbeTLBEntry(unsigned int EntryHi, unsigned int *PageMask, unsigned int *EntryLo0, unsigned int *EntryLo1);
extern int iProbeTLBEntry(unsigned int EntryHi, unsigned int *PageMask, unsigned int *EntryLo0, unsigned int *EntryLo1);
extern int ExpandScratchPad(unsigned int page);

extern void EnableIntcHandler(u32 cause);
extern void iEnableIntcHandler(u32 cause);
extern void DisableIntcHandler(u32 cause);
extern void iDisableIntcHandler(u32 cause);
extern void EnableDmacHandler(u32 channel);
extern void iEnableDmacHandler(u32 channel);
extern void DisableDmacHandler(u32 channel);
extern void iDisableDmacHandler(u32 channel);
extern void KSeg0(s32 arg1);
extern s32 EnableCache(s32 cache);
extern s32 DisableCache(s32 cache);
extern u32 GetCop0(s32 reg_id);
extern void FlushCache(s32 operation);
extern u32 CpuConfig(u32 config);
extern u32 iGetCop0(s32 reg_id);
extern void iFlushCache(s32 operation);
extern u32 iCpuConfig(u32 config);
extern void SetCPUTimerHandler(void (*handler)(void));
extern void SetCPUTimer(s32 compval);


extern void SetOsdConfigParam2(void *config, s32 size, s32 offset);
extern void GetOsdConfigParam2(void *config, s32 size, s32 offset);

extern u64 GsGetIMR(void);
extern u64 iGsGetIMR(void);
extern u64 GsPutIMR(u64 imr);
extern u64 iGsPutIMR(u64 imr);
extern void SetPgifHandler(void *handler);
extern void SetVSyncFlag(u32 *, u64 *);
extern void SetSyscall(s32 syscall_num, void *handler);
extern void _print(const char *fmt, ...);

extern void SifStopDma(void);

extern s32 SifDmaStat(u32 id);
extern s32 iSifDmaStat(u32 id);
extern u32 SifSetDma(SifDmaTransfer_t *sdd, s32 len);
extern u32 iSifSetDma(SifDmaTransfer_t *sdd, s32 len);


extern void SifSetDChain(void);
extern void iSifSetDChain(void);


extern int SifSetReg(u32 register_num, int register_value);
extern int SifGetReg(u32 register_num);

extern void _ExecOSD(int num_args, char *args[]) __attribute__((noreturn));
extern s32 Deci2Call(s32, u32 *);
extern void PSMode(void);
extern s32 MachineType(void);
extern s32 GetMemorySize(void);


extern void _GetGsDxDyOffset(int mode, int *dx, int *dy, int *dw, int *dh);


extern int _InitTLB(void);


extern int SetMemoryMode(int mode);

extern void _SyncDCache(void *start, void *end);
extern void _InvalidDCache(void *start, void *end);

extern void *GetSyscallHandler(int syscall_no);
extern void *GetExceptionHandler(int except_no);
extern void *GetInterruptHandler(int intr_no);


extern int kCopy(void *dest, const void *src, int size);
extern int kCopyBytes(void *dest, const void *src, int size);
extern int Copy(void *dest, const void *src, int size);
extern void setup(int syscall_num, void *handler);
extern void *GetEntryAddress(int syscall);
# 13 "main.c" 2
# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/stdlib.h" 1 3
# 10 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/stdlib.h" 3
# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/ieeefp.h" 1 3
# 11 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/stdlib.h" 2 3
# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/_ansi.h" 1 3
# 10 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/_ansi.h" 3
# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/newlib.h" 1 3
# 10 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/newlib.h" 3
# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/_newlib_version.h" 1 3
# 11 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/newlib.h" 2 3
# 11 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/_ansi.h" 2 3
# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/config.h" 1 3



# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/ieeefp.h" 1 3
# 5 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/config.h" 2 3
# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/features.h" 1 3
# 6 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/config.h" 2 3
# 12 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/_ansi.h" 2 3
# 12 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/stdlib.h" 2 3




# 1 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stddef.h" 1 3 4
# 17 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/stdlib.h" 2 3

# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 1 3
# 13 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 3
# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/_ansi.h" 1 3
# 14 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 2 3
# 1 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stddef.h" 1 3 4
# 15 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 2 3
# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/cdefs.h" 1 3
# 45 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/cdefs.h" 3
# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/_default_types.h" 1 3
# 41 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/_default_types.h" 3

# 41 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/_default_types.h" 3
typedef signed char __int8_t;

typedef unsigned char __uint8_t;
# 55 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/_default_types.h" 3
typedef short int __int16_t;

typedef short unsigned int __uint16_t;
# 77 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/_default_types.h" 3
typedef long int __int32_t;

typedef long unsigned int __uint32_t;
# 103 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/_default_types.h" 3
typedef long long int __int64_t;

typedef long long unsigned int __uint64_t;
# 134 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/_default_types.h" 3
typedef signed char __int_least8_t;

typedef unsigned char __uint_least8_t;
# 160 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/_default_types.h" 3
typedef short int __int_least16_t;

typedef short unsigned int __uint_least16_t;
# 182 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/_default_types.h" 3
typedef long int __int_least32_t;

typedef long unsigned int __uint_least32_t;
# 200 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/_default_types.h" 3
typedef long long int __int_least64_t;

typedef long long unsigned int __uint_least64_t;
# 214 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/_default_types.h" 3
typedef long long int __intmax_t;







typedef long long unsigned int __uintmax_t;







typedef int __intptr_t;

typedef unsigned int __uintptr_t;
# 46 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/cdefs.h" 2 3

# 1 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stddef.h" 1 3 4
# 48 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/cdefs.h" 2 3
# 16 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 2 3
# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/_types.h" 1 3
# 24 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/_types.h" 3
# 1 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stddef.h" 1 3 4
# 359 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stddef.h" 3 4
typedef unsigned int wint_t;
# 25 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/_types.h" 2 3


# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/_types.h" 1 3





typedef __int64_t _off_t;
# 28 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/_types.h" 2 3


typedef long __blkcnt_t;



typedef long __blksize_t;



typedef __uint64_t __fsblkcnt_t;



typedef __uint32_t __fsfilcnt_t;
# 52 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/_types.h" 3
typedef int __pid_t;



typedef short __dev_t;



typedef unsigned short __uid_t;


typedef unsigned short __gid_t;



typedef __uint32_t __id_t;







typedef unsigned short __ino_t;
# 90 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/_types.h" 3
typedef __uint32_t __mode_t;





__extension__ typedef long long _off64_t;





typedef _off_t __off_t;


typedef _off64_t __loff_t;


typedef long __key_t;







typedef long _fpos_t;
# 131 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/_types.h" 3
typedef unsigned int __size_t;
# 147 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/_types.h" 3
typedef signed int _ssize_t;
# 158 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/_types.h" 3
typedef _ssize_t __ssize_t;



typedef struct
{
  int __count;
  union
  {
    wint_t __wch;
    unsigned char __wchb[4];
  } __value;
} _mbstate_t;




typedef void *_iconv_t;






typedef unsigned long __clock_t;






typedef __int_least64_t __time_t;





typedef unsigned long __clockid_t;


typedef long __daddr_t;



typedef unsigned long __timer_t;


typedef __uint8_t __sa_family_t;



typedef __uint32_t __socklen_t;


typedef int __nl_item;
typedef unsigned short __nlink_t;
typedef long __suseconds_t;
typedef unsigned long __useconds_t;







typedef __builtin_va_list __va_list;
# 17 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 2 3






typedef unsigned long __ULong;
# 35 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 3
# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/lock.h" 1 3
# 33 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/lock.h" 3
struct __lock;
typedef struct __lock * _LOCK_T;






extern void __retarget_lock_init(_LOCK_T *lock);

extern void __retarget_lock_init_recursive(_LOCK_T *lock);

extern void __retarget_lock_close(_LOCK_T lock);

extern void __retarget_lock_close_recursive(_LOCK_T lock);

extern void __retarget_lock_acquire(_LOCK_T lock);

extern void __retarget_lock_acquire_recursive(_LOCK_T lock);

extern int __retarget_lock_try_acquire(_LOCK_T lock);

extern int __retarget_lock_try_acquire_recursive(_LOCK_T lock);


extern void __retarget_lock_release(_LOCK_T lock);

extern void __retarget_lock_release_recursive(_LOCK_T lock);
# 36 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 2 3
typedef _LOCK_T _flock_t;







struct _reent;

struct __locale_t;






struct _Bigint
{
  struct _Bigint *_next;
  int _k, _maxwds, _sign, _wds;
  __ULong _x[1];
};


struct __tm
{
  int __tm_sec;
  int __tm_min;
  int __tm_hour;
  int __tm_mday;
  int __tm_mon;
  int __tm_year;
  int __tm_wday;
  int __tm_yday;
  int __tm_isdst;
};







struct _on_exit_args {
 void * _fnargs[32];
 void * _dso_handle[32];

 __ULong _fntypes;


 __ULong _is_cxa;
};
# 99 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 3
struct _atexit {
 struct _atexit *_next;
 int _ind;

 void (*_fns[32])(void);
        struct _on_exit_args _on_exit_args;
};
# 116 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 3
struct __sbuf {
 unsigned char *_base;
 int _size;
};
# 153 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 3
struct __sFILE {
  unsigned char *_p;
  int _r;
  int _w;
  short _flags;
  short _file;
  struct __sbuf _bf;
  int _lbfsize;






  void * _cookie;

  int (*_read) (struct _reent *, void *,
        char *, int);
  int (*_write) (struct _reent *, void *,
         const char *,
         int);
  _fpos_t (*_seek) (struct _reent *, void *, _fpos_t, int);
  int (*_close) (struct _reent *, void *);


  struct __sbuf _ub;
  unsigned char *_up;
  int _ur;


  unsigned char _ubuf[3];
  unsigned char _nbuf[1];


  struct __sbuf _lb;


  int _blksize;
  _off_t _offset;


  struct _reent *_data;



  _flock_t _lock;

  _mbstate_t _mbstate;
  int _flags2;
};
# 270 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 3
typedef struct __sFILE __FILE;



extern __FILE __sf[3];

struct _glue
{
  struct _glue *_next;
  int _niobs;
  __FILE *_iobs;
};

extern struct _glue __sglue;
# 306 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 3
struct _rand48 {
  unsigned short _seed[3];
  unsigned short _mult[3];
  unsigned short _add;




};
# 568 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 3
struct _reent
{
  int _errno;




  __FILE *_stdin, *_stdout, *_stderr;

  int _inc;
  char _emergency[25];




  struct __locale_t *_locale;





  void (*__cleanup) (struct _reent *);


  struct _Bigint *_result;
  int _result_k;
  struct _Bigint *_p5s;
  struct _Bigint **_freelist;


  int _cvtlen;
  char *_cvtbuf;

  union
    {
      struct
        {



          char * _strtok_last;
          char _asctime_buf[26];
          struct __tm _localtime_buf;
          int _gamma_signgam;
          __extension__ unsigned long long _rand_next;
          struct _rand48 _r48;
          _mbstate_t _mblen_state;
          _mbstate_t _mbtowc_state;
          _mbstate_t _wctomb_state;
          char _l64a_buf[8];
          char _signal_buf[24];
          int _getdate_err;
          _mbstate_t _mbrlen_state;
          _mbstate_t _mbrtowc_state;
          _mbstate_t _mbsrtowcs_state;
          _mbstate_t _wcrtomb_state;
          _mbstate_t _wcsrtombs_state;
   int _h_errno;
# 634 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 3
        } _reent;







    } _new;







  void (**_sig_func)(int);
};
# 782 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 3
extern struct _reent *_impure_ptr __attribute__((__section__(".sdata")));





extern struct _reent _impure_data ;
# 900 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/sys/reent.h" 3
extern struct _atexit *__atexit;
extern struct _atexit __atexit0;

extern void (*__stdio_exit_handler) (void);

void _reclaim_reent (struct _reent *);

extern int _fwalk_sglue (struct _reent *, int (*)(struct _reent *, __FILE *),
    struct _glue *);
# 19 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/stdlib.h" 2 3

# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/stdlib.h" 1 3
# 21 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/stdlib.h" 2 3

# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/alloca.h" 1 3
# 23 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/stdlib.h" 2 3
# 33 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/stdlib.h" 3


typedef struct
{
  int quot;
  int rem;
} div_t;

typedef struct
{
  long quot;
  long rem;
} ldiv_t;


typedef struct
{
  long long int quot;
  long long int rem;
} lldiv_t;




typedef int (*__compar_fn_t) (const void *, const void *);







int __locale_mb_cur_max (void);



void abort (void) __attribute__ ((__noreturn__));
int abs (int);

__uint32_t arc4random (void);
__uint32_t arc4random_uniform (__uint32_t);
void arc4random_buf (void *, size_t);

int atexit (void (*__func)(void));
double atof (const char *__nptr);

float atoff (const char *__nptr);

int atoi (const char *__nptr);
int _atoi_r (struct _reent *, const char *__nptr);
long atol (const char *__nptr);
long _atol_r (struct _reent *, const char *__nptr);
void * bsearch (const void *__key,
         const void *__base,
         size_t __nmemb,
         size_t __size,
         __compar_fn_t _compar);
void *calloc(size_t, size_t) __attribute__((__malloc__)) __attribute__((__warn_unused_result__))
      __attribute__((__alloc_size__(1, 2))) ;
div_t div (int __numer, int __denom);
void exit (int __status) __attribute__ ((__noreturn__));
void free (void *) ;
char * getenv (const char *__string);
char * _getenv_r (struct _reent *, const char *__string);



char * _findenv (const char *, int *);
char * _findenv_r (struct _reent *, const char *, int *);

extern char *suboptarg;
int getsubopt (char **, char * const *, char **);

long labs (long);
ldiv_t ldiv (long __numer, long __denom);
void *malloc(size_t) __attribute__((__malloc__)) __attribute__((__warn_unused_result__)) __attribute__((__alloc_size__(1))) ;
int mblen (const char *, size_t);
int _mblen_r (struct _reent *, const char *, size_t, _mbstate_t *);
int mbtowc (wchar_t *restrict, const char *restrict, size_t);
int _mbtowc_r (struct _reent *, wchar_t *restrict, const char *restrict, size_t, _mbstate_t *);
int wctomb (char *, wchar_t);
int _wctomb_r (struct _reent *, char *, wchar_t, _mbstate_t *);
size_t mbstowcs (wchar_t *restrict, const char *restrict, size_t);
size_t _mbstowcs_r (struct _reent *, wchar_t *restrict, const char *restrict, size_t, _mbstate_t *);
size_t wcstombs (char *restrict, const wchar_t *restrict, size_t);
size_t _wcstombs_r (struct _reent *, char *restrict, const wchar_t *restrict, size_t, _mbstate_t *);


char * mkdtemp (char *);






int mkstemp (char *);


int mkstemps (char *, int);


char * mktemp (char *) __attribute__ ((__deprecated__("the use of `mktemp' is dangerous; use `mkstemp' instead")));


char * _mkdtemp_r (struct _reent *, char *);
int _mkostemp_r (struct _reent *, char *, int);
int _mkostemps_r (struct _reent *, char *, int, int);
int _mkstemp_r (struct _reent *, char *);
int _mkstemps_r (struct _reent *, char *, int);
char * _mktemp_r (struct _reent *, char *) __attribute__ ((__deprecated__("the use of `mktemp' is dangerous; use `mkstemp' instead")));
void qsort (void *__base, size_t __nmemb, size_t __size, __compar_fn_t _compar);
int rand (void);
void *realloc(void *, size_t) __attribute__((__warn_unused_result__)) __attribute__((__alloc_size__(2))) ;

void *reallocarray(void *, size_t, size_t) __attribute__((__warn_unused_result__)) __attribute__((__alloc_size__(2, 3)));
void *reallocf(void *, size_t) __attribute__((__warn_unused_result__)) __attribute__((__alloc_size__(2)));


char * realpath (const char *restrict path, char *restrict resolved_path);


int rpmatch (const char *response);




void srand (unsigned __seed);
double strtod (const char *restrict __n, char **restrict __end_PTR);
double _strtod_r (struct _reent *,const char *restrict __n, char **restrict __end_PTR);

float strtof (const char *restrict __n, char **restrict __end_PTR);







long strtol (const char *restrict __n, char **restrict __end_PTR, int __base);
long _strtol_r (struct _reent *,const char *restrict __n, char **restrict __end_PTR, int __base);
unsigned long strtoul (const char *restrict __n, char **restrict __end_PTR, int __base);
unsigned long _strtoul_r (struct _reent *,const char *restrict __n, char **restrict __end_PTR, int __base);
# 191 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/stdlib.h" 3
int system (const char *__string);


long a64l (const char *__input);
char * l64a (long __input);
char * _l64a_r (struct _reent *,long __input);


int on_exit (void (*__func)(int, void *),void *__arg);


void _Exit (int __status) __attribute__ ((__noreturn__));


int putenv (char *__string);

int _putenv_r (struct _reent *, char *__string);
void * _reallocf_r (struct _reent *, void *, size_t);

int setenv (const char *__string, const char *__value, int __overwrite);

int _setenv_r (struct _reent *, const char *__string, const char *__value, int __overwrite);
# 224 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/stdlib.h" 3
char * __itoa (int, char *, int);
char * __utoa (unsigned, char *, int);

char * itoa (int, char *, int);
char * utoa (unsigned, char *, int);


int rand_r (unsigned *__seed);



double drand48 (void);
double _drand48_r (struct _reent *);
double erand48 (unsigned short [3]);
double _erand48_r (struct _reent *, unsigned short [3]);
long jrand48 (unsigned short [3]);
long _jrand48_r (struct _reent *, unsigned short [3]);
void lcong48 (unsigned short [7]);
void _lcong48_r (struct _reent *, unsigned short [7]);
long lrand48 (void);
long _lrand48_r (struct _reent *);
long mrand48 (void);
long _mrand48_r (struct _reent *);
long nrand48 (unsigned short [3]);
long _nrand48_r (struct _reent *, unsigned short [3]);
unsigned short *
       seed48 (unsigned short [3]);
unsigned short *
       _seed48_r (struct _reent *, unsigned short [3]);
void srand48 (long);
void _srand48_r (struct _reent *, long);


char * initstate (unsigned, char *, size_t);
long random (void);
char * setstate (char *);
void srandom (unsigned);


long long atoll (const char *__nptr);

long long _atoll_r (struct _reent *, const char *__nptr);

long long llabs (long long);
lldiv_t lldiv (long long __numer, long long __denom);
long long strtoll (const char *restrict __n, char **restrict __end_PTR, int __base);

long long _strtoll_r (struct _reent *, const char *restrict __n, char **restrict __end_PTR, int __base);

unsigned long long strtoull (const char *restrict __n, char **restrict __end_PTR, int __base);

unsigned long long _strtoull_r (struct _reent *, const char *restrict __n, char **restrict __end_PTR, int __base);



void cfree (void *);


int unsetenv (const char *__string);

int _unsetenv_r (struct _reent *, const char *__string);



int posix_memalign (void **, size_t, size_t) __attribute__((__nonnull__ (1)))
     __attribute__((__warn_unused_result__));


char * _dtoa_r (struct _reent *, double, int, int, int *, int*, char**);

void * _malloc_r (struct _reent *, size_t) ;
void * _calloc_r (struct _reent *, size_t, size_t) ;
void _free_r (struct _reent *, void *) ;
void * _realloc_r (struct _reent *, void *, size_t) ;
void _mstats_r (struct _reent *, char *);

int _system_r (struct _reent *, const char *);

void __eprintf (const char *, const char *, unsigned int, const char *);
# 312 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/stdlib.h" 3
void qsort_r (void *__base, size_t __nmemb, size_t __size, void *__thunk, int (*_compar)(void *, const void *, const void *))
             __asm__ ("" "__bsd_qsort_r");
# 322 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/stdlib.h" 3
extern long double _strtold_r (struct _reent *, const char *restrict, char **restrict);

extern long double strtold (const char *restrict, char **restrict);







void * aligned_alloc(size_t, size_t) __attribute__((__malloc__)) __attribute__((__alloc_align__(1)))
     __attribute__((__alloc_size__(2))) __attribute__((__warn_unused_result__));
int at_quick_exit(void (*)(void));
_Noreturn void
 quick_exit(int);



# 14 "main.c" 2
# 1 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stdbool.h" 1 3 4
# 15 "main.c" 2
# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/malloc.h" 1 3
# 10 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/malloc.h" 3
# 1 "/usr/local/ps2dev/ee/lib/gcc/mips64r5900el-ps2-elf/14.2.0/include/stddef.h" 1 3 4
# 11 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/malloc.h" 2 3


# 1 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/machine/malloc.h" 1 3
# 14 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/malloc.h" 2 3
# 22 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/malloc.h" 3
struct mallinfo {
  size_t arena;
  size_t ordblks;
  size_t smblks;
  size_t hblks;
  size_t hblkhd;
  size_t usmblks;
  size_t fsmblks;
  size_t uordblks;
  size_t fordblks;
  size_t keepcost;
};



extern void *malloc (size_t);




extern void *_malloc_r (struct _reent *, size_t);


extern void free (void *);




extern void _free_r (struct _reent *, void *);


extern void *realloc (void *, size_t);




extern void *_realloc_r (struct _reent *, void *, size_t);


extern void *calloc (size_t, size_t);




extern void *_calloc_r (struct _reent *, size_t, size_t);


extern void *memalign (size_t, size_t);




extern void *_memalign_r (struct _reent *, size_t, size_t);


extern struct mallinfo mallinfo (void);




extern struct mallinfo _mallinfo_r (struct _reent *);


extern void malloc_stats (void);




extern void _malloc_stats_r (struct _reent *);


extern int mallopt (int, int);




extern int _mallopt_r (struct _reent *, int, int);


extern size_t malloc_usable_size (void *);




extern size_t _malloc_usable_size_r (struct _reent *, void *);





extern void *valloc (size_t);




extern void *_valloc_r (struct _reent *, size_t);


extern void *pvalloc (size_t);




extern void *_pvalloc_r (struct _reent *, size_t);


extern int malloc_trim (size_t);




extern int _malloc_trim_r (struct _reent *, size_t);


extern void __malloc_lock(struct _reent *);

extern void __malloc_unlock(struct _reent *);



extern void mstats (char *);




extern void _mstats_r (struct _reent *, char *);
# 166 "/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/include/malloc.h" 3
extern void cfree (void *);
# 16 "main.c" 2

# 1 "/usr/local/ps2dev/ps2sdk/ee/include/math3d.h" 1
# 21 "/usr/local/ps2dev/ps2sdk/ee/include/math3d.h"

# 21 "/usr/local/ps2dev/ps2sdk/ee/include/math3d.h"
typedef float VECTOR[4] __attribute__((__aligned__(16)));

typedef float MATRIX[16] __attribute__((__aligned__(16)));







extern void vector_apply(VECTOR output, VECTOR input0, MATRIX input1);


extern void vector_clamp(VECTOR output, VECTOR input0, float min, float max);


extern void vector_copy(VECTOR output, VECTOR input0);


extern float vector_innerproduct(VECTOR input0, VECTOR input1);


extern void vector_multiply(VECTOR output, VECTOR input0, VECTOR input1);


extern void vector_normalize(VECTOR output, VECTOR input0);


extern void vector_outerproduct(VECTOR output, VECTOR input0, VECTOR input1);


extern void vector_add(VECTOR sum, VECTOR addend, VECTOR summand);


extern void vector_cross_product(VECTOR product, VECTOR multiplicand, VECTOR multiplier);


extern void vector_triangle_normal(VECTOR output, VECTOR a, VECTOR b, VECTOR c);



extern void matrix_copy(MATRIX output, MATRIX input0);


extern void matrix_inverse(MATRIX output, MATRIX input0);


extern void matrix_multiply(MATRIX output, MATRIX input0, MATRIX input1);


extern void matrix_rotate(MATRIX output, MATRIX input0, VECTOR input1);


extern void matrix_scale(MATRIX output, MATRIX input0, VECTOR input1);


extern void matrix_translate(MATRIX output, MATRIX input0, VECTOR input1);


extern void matrix_transpose(MATRIX output, MATRIX input0);


extern void matrix_unit(MATRIX output);






extern void create_local_world(MATRIX local_world, VECTOR translation, VECTOR rotation);




extern void create_local_light(MATRIX local_light, VECTOR rotation);




extern void create_world_view(MATRIX world_view, VECTOR translation, VECTOR rotation);




extern void create_view_screen(MATRIX view_screen, float aspect, float left, float right, float bottom, float top, float near, float far);




extern void create_local_screen(MATRIX local_screen, MATRIX local_world, MATRIX world_view, MATRIX view_screen);
# 120 "/usr/local/ps2dev/ps2sdk/ee/include/math3d.h"
extern void calculate_normals(VECTOR *output, int count, VECTOR *normals, MATRIX local_light);


extern void calculate_lights(VECTOR *output, int count, VECTOR *normals, VECTOR *light_directions, VECTOR *light_colours, const int *light_types, int light_count);


extern void calculate_colours(VECTOR *output, int count, VECTOR *colours, VECTOR *lights);


extern void calculate_vertices(VECTOR *output, int count, VECTOR *vertices, MATRIX local_screen);
# 18 "main.c" 2

# 1 "/usr/local/ps2dev/ps2sdk/ee/include/packet.h" 1
# 23 "/usr/local/ps2dev/ps2sdk/ee/include/packet.h"
typedef struct {
 u32 qwords;
 u16 qwc;
 u16 type;
 qword_t *data __attribute__((aligned(64)));
} packet_t;






extern packet_t *packet_init(int qwords, int type);


extern void packet_reset(packet_t *packet);


extern void packet_free(packet_t *packet);


extern qword_t *packet_increment_qwc(packet_t *packet, int num);


static inline qword_t *packet_get_qword(packet_t *packet)
{
 return packet->data;
}
# 20 "main.c" 2

# 1 "/usr/local/ps2dev/ps2sdk/ee/include/dma_tags.h" 1
# 22 "main.c" 2
# 1 "/usr/local/ps2dev/ps2sdk/common/include/gif_tags.h" 1
# 23 "main.c" 2
# 1 "/usr/local/ps2dev/ps2sdk/common/include/gs_psm.h" 1
# 24 "main.c" 2

# 1 "/usr/local/ps2dev/ps2sdk/ee/include/dma.h" 1
# 20 "/usr/local/ps2dev/ps2sdk/ee/include/dma.h"
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/packet2_types.h" 1
# 31 "/usr/local/ps2dev/ps2sdk/ee/include/packet2_types.h"
enum Packet2Mode
{
    P2_MODE_NORMAL = 0,
    P2_MODE_CHAIN = 1,
};


enum Packet2Type
{

    P2_TYPE_NORMAL = 0x00000000,

    P2_TYPE_UNCACHED = 0x20000000,

    P2_TYPE_UNCACHED_ACCL = 0x30000000,

    P2_TYPE_SPRAM = 0x70000000
};


enum DmaTagType
{

    P2_DMA_TAG_REFE = 0,

    P2_DMA_TAG_CNT = 1,

    P2_DMA_TAG_NEXT = 2,

    P2_DMA_TAG_REF = 3,





    P2_DMA_TAG_REFS = 4,






    P2_DMA_TAG_CALL = 5,
# 83 "/usr/local/ps2dev/ps2sdk/ee/include/packet2_types.h"
    P2_DMA_TAG_RET = 6,

    P2_DMA_TAG_END = 7
};


typedef struct
{

    u64 QWC : 16;
    u64 PAD : 10;







    u64 PCE : 2;

    u64 ID : 3;





    u64 IRQ : 1;





    u64 ADDR : 31;





    u64 SPR : 1;
    u32 OPT1;
    u32 OPT2;
} dma_tag_t __attribute__((aligned(16)));






enum UnpackMode
{
    P2_UNPACK_S_32 = 0x00,
    P2_UNPACK_S_16 = 0x01,
    P2_UNPACK_S_8 = 0x02,
    P2_UNPACK_V2_32 = 0x04,
    P2_UNPACK_V2_16 = 0x05,
    P2_UNPACK_V2_8 = 0x06,
    P2_UNPACK_V3_32 = 0x08,
    P2_UNPACK_V3_16 = 0x09,
    P2_UNPACK_V3_8 = 0x0A,
    P2_UNPACK_V4_32 = 0x0C,
    P2_UNPACK_V4_16 = 0x0D,
    P2_UNPACK_V4_8 = 0x0E,
    P2_UNPACK_V4_5 = 0x0F,
};


enum VIFOpcode
{





    P2_VIF_NOP = 0,




    P2_VIF_STCYCL = 1,






    P2_VIF_OFFSET = 2,





    P2_VIF_BASE = 3,





    P2_VIF_ITOP = 4,





    P2_VIF_STMOD = 5,





    P2_VIF_MSKPATH3 = 6,






    P2_VIF_MARK = 7,




    P2_VIF_FLUSHE = 16,






    P2_VIF_FLUSH = 17,





    P2_VIF_FLUSHA = 19,





    P2_VIF_MSCAL = 20,





    P2_VIF_MSCNT = 23,





    P2_VIF_MSCALF = 21,





    P2_VIF_STMASK = 32,





    P2_VIF_STROW = 48,





    P2_VIF_STCOL = 49,





    P2_VIF_MPG = 74,





    P2_VIF_DIRECT = 80,
# 274 "/usr/local/ps2dev/ps2sdk/ee/include/packet2_types.h"
    P2_VIF_DIRECTHL = 81
};


typedef struct
{

    u32 immediate : 16;






    u32 num : 8;




    u32 cmd : 8;
} vif_code_t;





typedef struct
{

    u16 max_qwords_count;

    enum Packet2Type type;

    enum Packet2Mode mode;







    u8 tte;

    qword_t *base __attribute__((aligned(64)));

    qword_t *next;




    dma_tag_t *tag_opened_at;




    vif_code_t *vif_code_opened_at;
} packet2_t;


typedef union
{
    struct
    {
        unsigned int m0 : 2;
        unsigned int m1 : 2;
        unsigned int m2 : 2;
        unsigned int m3 : 2;
        unsigned int m4 : 2;
        unsigned int m5 : 2;
        unsigned int m6 : 2;
        unsigned int m7 : 2;
        unsigned int m8 : 2;
        unsigned int m9 : 2;
        unsigned int m10 : 2;
        unsigned int m11 : 2;
        unsigned int m12 : 2;
        unsigned int m13 : 2;
        unsigned int m14 : 2;
        unsigned int m15 : 2;
    };
    unsigned int m;
} Mask;
# 21 "/usr/local/ps2dev/ps2sdk/ee/include/dma.h" 2
# 40 "/usr/local/ps2dev/ps2sdk/ee/include/dma.h"
extern int dma_reset(void);


extern int dma_channel_initialize(int channel, void *handler, int flags);


extern void dma_channel_fast_waits(int channel);


extern void dma_wait_fast(void);


extern int dma_channel_wait(int channel, int timeout);
# 61 "/usr/local/ps2dev/ps2sdk/ee/include/dma.h"
extern void dma_channel_send_packet2(packet2_t *packet2, int channel, u8 flush_cache);


extern int dma_channel_send_chain(int channel, void *data, int qwc, int flags, int spr);


extern int dma_channel_send_chain_ucab(int channel, void *data, int qwc, int flags);


extern int dma_channel_send_normal(int channel, void *data, int qwc, int flags, int spr);


extern int dma_channel_send_normal_ucab(int channel, void *data, int qwc, int flags);


extern int dma_channel_receive_normal(int channel, void *data, int data_size, int flags, int spr);


extern int dma_channel_receive_chain(int channel, void *data, int data_size, int flags, int spr);


extern int dma_channel_shutdown(int channel, int flags);
# 26 "main.c" 2

# 1 "/usr/local/ps2dev/ps2sdk/ee/include/graph.h" 1
# 9 "/usr/local/ps2dev/ps2sdk/ee/include/graph.h"
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/graph_vram.h" 1
# 23 "/usr/local/ps2dev/ps2sdk/ee/include/graph_vram.h"
extern int graph_vram_allocate(int width, int height, int psm, int alignment);


extern void graph_vram_free(int address);


extern void graph_vram_clear(void);


extern int graph_vram_size(int width, int height, int psm, int alignment);
# 10 "/usr/local/ps2dev/ps2sdk/ee/include/graph.h" 2
# 89 "/usr/local/ps2dev/ps2sdk/ee/include/graph.h"
typedef struct {
 int x,y;
 int width, height;
 int mode;
} GRAPH_MODE;

extern GRAPH_MODE graph_mode[];






extern int graph_initialize(int fbp, int width, int height, int psm, int x, int y);


extern int graph_get_region(void);


extern float graph_aspect_ratio(void);


extern void graph_enable_output(void);


extern void graph_disable_output(void);


extern int graph_set_mode(int interlace, int mode, int ffmd, int flicker_filter);


extern int graph_set_screen(int x, int y, int width, int height);


extern void graph_set_framebuffer_filtered(int fbp, int width, int psm, int x, int y);


extern void graph_set_framebuffer(int context, int fbp, int width, int psm, int x, int y);


extern void graph_set_bgcolor(unsigned char r, unsigned char g, unsigned char b);


extern void graph_set_output(int rc1, int rc2, int alpha_select, int alpha_output, int blend_method, unsigned char alpha);


extern int graph_add_vsync_handler(int (*vsync_callback)());


extern void graph_remove_vsync_handler(int callback_id);


extern void graph_wait_hsync(void);


extern void graph_wait_vsync(void);


extern int graph_check_vsync(void);


extern void graph_start_vsync(void);


extern int graph_shutdown(void);
# 28 "main.c" 2

# 1 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h" 1
# 11 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h"
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/draw_blending.h" 1
# 26 "/usr/local/ps2dev/ps2sdk/ee/include/draw_blending.h"
typedef struct {
 char color1;
 char color2;
 char alpha;
 char color3;
 unsigned char fixed_alpha;
} blend_t;






extern qword_t *draw_pixel_alpha_control(qword_t *q, int enable);


extern qword_t *draw_alpha_blending(qword_t *q, int context, blend_t *blend);


extern qword_t *draw_alpha_correction(qword_t *q, int context, int alpha);
# 12 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h" 2
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/draw_buffers.h" 1
# 33 "/usr/local/ps2dev/ps2sdk/ee/include/draw_buffers.h"
typedef struct {
 unsigned char width;
 unsigned char height;
 unsigned char components;
 unsigned char function;
} texinfo_t;

typedef struct {
 unsigned int address;
 unsigned int width;
 unsigned int height;
 unsigned int psm;
 unsigned int mask;
} framebuffer_t;

typedef struct {
 unsigned int enable;
 unsigned int method;
 unsigned int address;
 unsigned int zsm;
 unsigned int mask;
} zbuffer_t;

typedef struct {
 unsigned int address;
 unsigned int width;
 unsigned int psm;
 texinfo_t info;
} texbuffer_t;

typedef struct {
 unsigned int address;
 unsigned int psm;
 unsigned int storage_mode;
 unsigned int start;
 unsigned int load_method;
} clutbuffer_t;






extern unsigned char draw_log2(unsigned int x);


extern qword_t *draw_framebuffer(qword_t *q, int context, framebuffer_t *frame);


extern qword_t *draw_zbuffer(qword_t *q, int context, zbuffer_t *zbuffer);


extern qword_t *draw_texturebuffer(qword_t *q, int context, texbuffer_t *texbuffer, clutbuffer_t *clut);


extern qword_t *draw_clutbuffer(qword_t *q, int context, int psm, clutbuffer_t *clut);


extern qword_t *draw_clut_offset(qword_t *q, int cbw, int u, int v);
# 13 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h" 2
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/draw_dithering.h" 1
# 11 "/usr/local/ps2dev/ps2sdk/ee/include/draw_dithering.h"
typedef signed char dithermx_t[16];






extern qword_t *draw_dithering(qword_t *q, int enable);


extern qword_t *draw_dither_matrix(qword_t *q,char *dm);
# 14 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h" 2
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/draw_fog.h" 1
# 16 "/usr/local/ps2dev/ps2sdk/ee/include/draw_fog.h"
extern qword_t *draw_fog_color(qword_t *q, unsigned char r, unsigned char g, unsigned char b);
# 15 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h" 2
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/draw_masking.h" 1
# 25 "/usr/local/ps2dev/ps2sdk/ee/include/draw_masking.h"
extern qword_t *draw_scan_masking(qword_t *q, int mask);


extern qword_t *draw_color_clamping(qword_t *q, int enable);
# 16 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h" 2
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/draw_primitives.h" 1
# 36 "/usr/local/ps2dev/ps2sdk/ee/include/draw_primitives.h"
typedef struct {
 unsigned char type;
 unsigned char shading;
 unsigned char mapping;
 unsigned char fogging;
 unsigned char blending;
 unsigned char antialiasing;
 unsigned char mapping_type;
 unsigned char colorfix;
} prim_t;






extern qword_t *draw_primitive_xyoffset(qword_t *q, int context, float x, float y);


extern qword_t *draw_primitive_override(qword_t *q, int mode);


extern qword_t *draw_primitive_override_setting(qword_t *q, int context, prim_t *prim);
# 17 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h" 2
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/draw_sampling.h" 1
# 39 "/usr/local/ps2dev/ps2sdk/ee/include/draw_sampling.h"
typedef struct {
 unsigned char calculation;
 unsigned char max_level;
 unsigned char mag_filter;
 unsigned char min_filter;
 unsigned char mipmap_select;
 unsigned char l;
 float k;
} lod_t;

typedef struct {
 int address1;
 int address2;
 int address3;
 char width1;
 char width2;
 char width3;
} mipmap_t;

typedef struct {
 unsigned char horizontal;
 unsigned char vertical;
 int minu, maxu;
 int minv, maxv;
} texwrap_t;






extern qword_t *draw_texture_sampling(qword_t *q, int context, lod_t *lod);


extern qword_t *draw_mipmap1(qword_t *q, int context, mipmap_t *mipmap);


extern qword_t *draw_mipmap2(qword_t *q, int context, mipmap_t *mipmap);


extern qword_t *draw_texture_wrapping(qword_t *q, int context, texwrap_t *wrap);


extern qword_t *draw_texture_expand_alpha(qword_t *q, unsigned char zero_value, int expand, unsigned char one_value);
# 18 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h" 2
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/draw_tests.h" 1
# 37 "/usr/local/ps2dev/ps2sdk/ee/include/draw_tests.h"
typedef struct {
 unsigned char enable;
 unsigned char method;
 unsigned char compval;
 unsigned char keep;
} atest_t;

typedef struct {
 unsigned char enable;
 unsigned char pass;
} dtest_t;

typedef struct {
 unsigned char enable;
 unsigned char method;
} ztest_t;






extern qword_t *draw_scissor_area(qword_t *q, int context, int x0, int x1, int y0, int y1);


extern qword_t *draw_pixel_test(qword_t *q, int context, atest_t *atest, dtest_t *dtest, ztest_t *ztest);


extern qword_t *draw_disable_tests(qword_t *q, int context, zbuffer_t *z);


extern qword_t *draw_enable_tests(qword_t *q, int context, zbuffer_t *z);
# 19 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h" 2
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/draw_types.h" 1
# 18 "/usr/local/ps2dev/ps2sdk/ee/include/draw_types.h"
typedef union {
 u64 xyz;
 struct {
  u16 x;
  u16 y;
  u32 z;
 };
} __attribute__((packed,aligned(8))) xyz_t;

typedef union {
 u64 uv;
 struct {
  float s;
  float t;
 };
 struct {
  float u;
  float v;
 };
} __attribute__((packed,aligned(8))) texel_t;

typedef union {
 u64 rgbaq;
 struct {
  u8 r;
  u8 g;
  u8 b;
  u8 a;
  float q;
 };
} __attribute__((packed,aligned(8))) color_t;


typedef struct {
 float x;
 float y;
 unsigned int z;
} vertex_t;

typedef union {
 VECTOR strq;
 struct {
  float s;
  float t;
  float r;
  float q;
 };
} __attribute__((packed,aligned(16))) texel_f_t;

typedef union {
 VECTOR rgba;
 struct {
  float r;
  float g;
  float b;
  float a;
 };
} __attribute__((packed,aligned(16))) color_f_t;

typedef union {
 VECTOR xyzw;
 struct {
  float x;
  float y;
  float z;
  float w;
 };
} __attribute__((packed,aligned(16))) vertex_f_t;
# 20 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h" 2

# 1 "/usr/local/ps2dev/ps2sdk/ee/include/draw2d.h" 1
# 13 "/usr/local/ps2dev/ps2sdk/ee/include/draw2d.h"
typedef struct {
 vertex_t v0;
 color_t color;
} point_t;

typedef struct {
 vertex_t v0;
 vertex_t v1;
 color_t color;
} line_t;

typedef struct {
 vertex_t v0;
 vertex_t v1;
 vertex_t v2;
 color_t color;
} triangle_t;

typedef struct {
 vertex_t v0;
 vertex_t v1;
 color_t color;
} rect_t;

typedef struct {
 vertex_t v0;
 texel_t t0;
 vertex_t v1;
 texel_t t1;
 color_t color;
} texrect_t;






extern void draw_enable_blending();


extern void draw_disable_blending();


extern qword_t *draw_point(qword_t *q, int context, point_t *point);


extern qword_t *draw_line(qword_t *q, int context, line_t *line);


extern qword_t *draw_triangle_outline(qword_t *q, int context, triangle_t *triangle);


extern qword_t *draw_triangle_filled(qword_t *q, int context,triangle_t *triangle);


extern qword_t *draw_rect_outline(qword_t *q, int context, rect_t *rect);


extern qword_t *draw_rect_filled(qword_t *q, int context, rect_t *rect);


extern qword_t *draw_rect_textured(qword_t *q, int context, texrect_t *rect);


extern qword_t *draw_rect_filled_strips(qword_t *q, int context, rect_t *rect);




extern qword_t *draw_rect_textured_strips(qword_t *q, int context, texrect_t *rect);


extern qword_t *draw_round_rect_filled(qword_t *q, int context, rect_t *rect);


extern qword_t *draw_round_rect_outline(qword_t *q, int context, rect_t *rect);


extern qword_t *draw_arc_outline(qword_t *q, int context, point_t *center, float radius, float angle_start, float angle_end);


extern qword_t *draw_arc_filled(qword_t *q, int context, point_t *center, float radius, float angle_start, float angle_end);
# 22 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h" 2
# 1 "/usr/local/ps2dev/ps2sdk/ee/include/draw3d.h" 1
# 65 "/usr/local/ps2dev/ps2sdk/ee/include/draw3d.h"
extern qword_t *draw_prim_start(qword_t *q, int context, prim_t *prim, color_t *color);


extern qword_t *draw_prim_end(qword_t *q,int nreg, u64 reglist);


extern int draw_convert_rgbq(color_t *output, int count, vertex_f_t *vertices, color_f_t *colours, unsigned char alpha);


extern int draw_convert_rgbaq(color_t *output, int count, vertex_f_t *vertices, color_f_t *colours);


extern int draw_convert_st(texel_t *output, int count, vertex_f_t *vertices, texel_f_t *coords);


extern int draw_convert_xyz(xyz_t *output, float x, float y, int z, int count, vertex_f_t *vertices);
# 23 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h" 2
# 32 "/usr/local/ps2dev/ps2sdk/ee/include/draw.h"
extern qword_t *draw_setup_environment(qword_t *q, int context, framebuffer_t *frame, zbuffer_t *z);


extern qword_t *draw_clear(qword_t *q, int context, float x, float y, float width, float height, int r, int g, int b);


extern qword_t *draw_finish(qword_t *q);


extern void draw_wait_finish(void);


extern qword_t *draw_texture_transfer(qword_t *q, void *src, int width, int height, int psm, int dest, int dest_width);


extern qword_t *draw_texture_flush(qword_t *q);
# 30 "main.c" 2
# 40 "main.c"
int POINTS[6] = {
  0, 1, 2,
  1, 2, 3
};

VECTOR XYZ[4] = {
  { -10.0f, -10.0f, 10.0f, 1.00f },
  { 10.0f, -10.0f, 10.0f, 1.00f },
  { -10.0f, 10.0f, 10.0f, 1.00f },
  { 10.0f, 10.0f, 10.0f, 1.00f }
};

VECTOR ST[4] = {
  { 0.00f, 0.00f, 0.00f, 0.00f },
  { 1.00f, 0.00f, 0.00f, 0.00f },
  { 0.00f, 1.00f, 0.00f, 0.00f },
  { 1.00f, 1.00f, 0.00f, 0.00f }
};

unsigned int UV[4][4] = {
  { 0, 0, 0, 0 },
  { 64 << 4, 0, 0, 0 },
  { 0, 64 << 4, 0, 0 },
  { 64 << 4, 64 << 4, 0, 0 }
};
# 84 "main.c"
unsigned char checkerboard[64 * 64 * 3] __attribute__((aligned(16)));

VECTOR object_position = { 0.00f, 0.00f, 0.00f, 1.00f };
VECTOR object_rotation = { 0.00f, 0.00f, 0.00f, 1.00f };

VECTOR camera_position = { 0.00f, 0.00f, 100.00f, 1.00f };
VECTOR camera_rotation = { 0.00f, 0.00f, 0.00f, 1.00f };

void make_checkerboard() {

  int r = 0xF0;
  int g = 0xFF;
  int b = 0xF0;





  for (int x = 0; x < 64; x++) {
    for (int y = 0; y < 64; y++) {
      if ((x / 32) % 2) {
        checkerboard[3 * x * 64 + 3 * y] = 0xFF;
        checkerboard[3 * x * 64 + 3 * y + 1] = 0xFF;
        checkerboard[3 * x * 64 + 3 * y + 2] = 0xFF;
      } else {
        checkerboard[3 * x * 64 + 3 * y] = r;
        checkerboard[3 * x * 64 + 3 * y + 1] = g;
        checkerboard[3 * x * 64 + 3 * y + 2] = b;
      }
    }
  }
}

void init_gs(framebuffer_t* frame, zbuffer_t* z, texbuffer_t* texbuf)
{

  frame->width = 640;
  frame->height = 256;
  frame->mask = 0;
  frame->psm = 0x00;
  frame->address = graph_vram_allocate(frame->width, frame->height, frame->psm, 2048);


  z->enable = 1;
  z->mask = 0;
  z->method = 2;
  z->zsm = 0x00;
  z->address = graph_vram_allocate(frame->width, frame->height, z->zsm, 2048);


  texbuf->width = 64;
  texbuf->psm = 0x01;
  texbuf->address = graph_vram_allocate(64, 64, 0x01, 64);


  graph_initialize(frame->address, frame->width, frame->height, frame->psm, 0, 0);
}

void init_drawing_environment(framebuffer_t *frame, zbuffer_t *z)
{
  packet_t *packet = packet_init(20, 0x00);


  qword_t *q = packet->data;


  q = draw_setup_environment(q, 0, frame, z);


  q = draw_primitive_xyoffset(q, 0, (2048 - (640 / 2)), (2048 - (256 / 2)));


  q = draw_finish(q);


  dma_channel_send_normal(0x02, packet->data, q - packet->data, 0, 0);
  dma_wait_fast();

  packet_free(packet);
}

void load_texture(texbuffer_t *texbuf)
{
  packet_t *packet = packet_init(50, 0x00);

  qword_t *q;

  q = packet->data;

  q = draw_texture_transfer(q, checkerboard, 64, 64, 0x01, texbuf->address, texbuf->width);
  q = draw_texture_flush(q);

  dma_channel_send_chain(0x02, packet->data, q - packet->data, 0, 0);
  dma_wait_fast();

  packet_free(packet);
}

void setup_texture(texbuffer_t *texbuf)
{
  packet_t *packet = packet_init(10, 0x00);

  qword_t *q = packet->data;


  clutbuffer_t clut;

  lod_t lod;

  lod.calculation = 1;
  lod.max_level = 0;
  lod.mag_filter = 0;
  lod.min_filter = 0;
  lod.l = 0;
  lod.k = 0;

  texbuf->info.width = draw_log2(64);
  texbuf->info.height = draw_log2(64);
  texbuf->info.components = 0;
  texbuf->info.function = 1;

  clut.storage_mode = 0;
  clut.start = 0;
  clut.psm = 0;
  clut.load_method = 0;
  clut.address = 0;

  q = draw_texture_sampling(q, 0, &lod);
  q = draw_texturebuffer(q, 0, texbuf, &clut);


  dma_channel_send_normal(0x02, packet->data, q - packet->data, 0, 0);
  dma_wait_fast();

  packet_free(packet);
}

int get_dummy_rgbq(color_t* output, int count, vertex_f_t* vertices)
{
  float q = 1.00f;
  for (int i = 0; i < count; i++)
  {

    if (vertices[i].w != 0)
    {
      q = 1 / vertices[i].w;
    }

    output[i].r = 0xFF;
    output[i].g = 0xFF;
    output[i].b = 0xFF;
    output[i].a = 0x80;
    output[i].q = q;
  }
  return 0;
}

int render(framebuffer_t* frame, zbuffer_t* z)
{
  int context = 0;

  int rep_x = 4, rep_y = 4;
  int x = 0, y = 0;
  int shift = 0, max_shift = 100000;
  float dshift = 1 / (float)max_shift;

  packet_t *packets[2];
  packet_t *current;

  MATRIX local_world;
  MATRIX world_view;
  MATRIX view_screen;
  MATRIX local_screen;

  prim_t prim;
  color_t color;

  VECTOR* temp_vertices;

  xyz_t* xyz;
  color_t* rgbaq;
  texel_t* tex_coords;

  packets[0] = packet_init(100, 0x00);
  packets[1] = packet_init(100, 0x00);



  prim.type = 0x03;



  prim.shading = 1;
  prim.mapping = 1;
  prim.fogging = 0;
  prim.blending = 0;
  prim.antialiasing = 0;

  prim.mapping_type = 0;



  prim.colorfix = 0;

  color.r = 0x80;
  color.g = 0x80;
  color.b = 0x80;
  color.a = 0x80;
  color.q = 1.0f;


  temp_vertices = memalign(128, sizeof(VECTOR) * 4);


  xyz = memalign(128, sizeof(u64) * 4);
  rgbaq = memalign(128, sizeof(u64) * 4);
  tex_coords = memalign(128, sizeof(u64) * 4);


  create_view_screen(view_screen, graph_aspect_ratio(), -3.00f, 3.00f, -3.00f, 3.00f, 1.00f, 2000.00f);


  while (
# 306 "main.c" 3 4
        1
# 306 "main.c"
            )
  {
    current = packets[context];


    object_position[0] = (x - rep_x / 2 + 0.5) * 2 * 10.0f;
    object_position[1] = (y - rep_y / 2 + 0.5 + (shift * dshift)) * 2 * 10.0f;


    create_local_world(local_world, object_position, object_rotation);


    create_world_view(world_view, camera_position, camera_rotation);


    create_local_screen(local_screen, local_world, world_view, view_screen);


    calculate_vertices(temp_vertices, 4, XYZ, local_screen);


    draw_convert_xyz(xyz, 2048, 2048, 32, 4, (vertex_f_t*)temp_vertices);


    get_dummy_rgbq(rgbaq, 4, (vertex_f_t*)temp_vertices);



    draw_convert_st(tex_coords, 4, (vertex_f_t*)temp_vertices, (texel_f_t*)ST);
# 343 "main.c"
    qword_t* q = current->data;


    q = draw_disable_tests(q, 0, z);
    if (x == 0 && y == 0) {
      q = draw_clear(q, 0, 2048.0f - ((float)640 / 2), 2048.0f - ((float)256 / 2),
        frame->width, frame->height, 0xFF, 0, 0);
    }
    q = draw_enable_tests(q,0,z);



    u64* dw = (u64*)draw_prim_start(q, 0, &prim, &color);

    for(int i = 0; i < 6; i++)
    {
      *dw++ = rgbaq[POINTS[i]].rgbaq;
      *dw++ = tex_coords[POINTS[i]].uv;
      *dw++ = xyz[POINTS[i]].xyz;
    }


    if ((u32)dw % 16)
    {
      *dw++ = 0;
    }



    q = draw_prim_end((qword_t*)dw, 3, ((u64)0x01) << 0 | ((u64)0x02) << 4 | ((u64)0x05) << 8);





    q = draw_finish(q);


    dma_wait_fast();
    dma_channel_send_normal(0x02,current->data, q - current->data, 0, 0);

    if (x++ == rep_x)
    {
      x = 0;
      if (y++ == rep_y)
      {
        y = 0;

        context ^= 1;

        draw_wait_finish();

        graph_wait_vsync();
      }
    }
    shift = (shift + 1) % max_shift;
  }

  free(packets[0]);
  free(packets[1]);


  return 0;

}

int main(int argc, char *argv[])
{

  make_checkerboard();


  framebuffer_t frame;
  zbuffer_t z;
  texbuffer_t texbuf;


  dma_channel_initialize(0x02,
# 420 "main.c" 3 4
                                        ((void *)0)
# 420 "main.c"
                                            ,0);
  dma_channel_fast_waits(0x02);


  init_gs(&frame, &z, &texbuf);


  init_drawing_environment(&frame,&z);


  load_texture(&texbuf);


  setup_texture(&texbuf);


  render(&frame,&z);


  SleepThread();


  return 0;

}
