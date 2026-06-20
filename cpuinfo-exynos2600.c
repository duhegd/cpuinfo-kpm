#include <stddef.h>
#include <sys/syscall.h>

#ifndef __user
#define __user
#endif

#define KERN_INFO "[INFO] "
#define KERN_ERR  "[ERROR] "
void printk(const char *fmt, ...);

struct pt_regs {
    unsigned long regs[31];
    unsigned long sp;
    unsigned long pc;
    unsigned long pstate;
    unsigned long orig_x0;
    unsigned long syscallno;
    unsigned long unused2;
};

long strncpy_from_user(char *dst, const char __user *src, long count);
long copy_to_user(void __user *to, const void *from, unsigned long n);

#include <kpm.h>

#define CONTROL_TOKEN   "@huanjingnb66"

static const char fake_cpuinfo[] =
    "Processor       : AArch64 Processor rev 4 (aarch64)\n"
    "Features        : fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp cpuid asimdrdm lrcpc dcpop asimddp ssbs sme sme2\n"
    "CPU implementer : 0x51\n"
    "CPU architecture: 8\n"
    "CPU variant     : 0x4\n"
    "CPU part        : 0x2025\n"
    "CPU revision    : 0\n"
    "\n"
    "processor       : 0\n"
    "BogoMIPS        : 56.00\n"
    "CPU implementer : 0x51\n"
    "CPU architecture: 8\n"
    "CPU variant     : 0x4\n"
    "CPU part        : 0x2025\n"
    "CPU revision    : 0\n"
    "\n"
    "processor       : 1\n"
    "BogoMIPS        : 48.00\n"
    "CPU implementer : 0x51\n"
    "CPU architecture: 8\n"
    "CPU variant     : 0x3\n"
    "CPU part        : 0x2026\n"
    "CPU revision    : 0\n"
    "\n"
    "processor       : 2\n"
    "BogoMIPS        : 48.00\n"
    "CPU implementer : 0x51\n"
    "CPU architecture: 8\n"
    "CPU variant     : 0x3\n"
    "CPU part        : 0x2026\n"
    "CPU revision    : 0\n"
    "\n"
    "processor       : 3\n"
    "BogoMIPS        : 48.00\n"
    "CPU implementer : 0x51\n"
    "CPU architecture: 8\n"
    "CPU variant     : 0x3\n"
    "CPU part        : 0x2026\n"
    "CPU revision    : 0\n"
    "\n"
    "processor       : 4\n"
    "BogoMIPS        : 40.00\n"
    "CPU implementer : 0x51\n"
    "CPU architecture: 8\n"
    "CPU variant     : 0x2\n"
    "CPU part        : 0x2027\n"
    "CPU revision    : 0\n"
    "\n"
    "processor       : 5\n"
    "BogoMIPS        : 40.00\n"
    "CPU implementer : 0x51\n"
    "CPU architecture: 8\n"
    "CPU variant     : 0x2\n"
    "CPU part        : 0x2027\n"
    "CPU revision    : 0\n"
    "\n"
    "processor       : 6\n"
    "BogoMIPS        : 40.00\n"
    "CPU implementer : 0x51\n"
    "CPU architecture: 8\n"
    "CPU variant     : 0x2\n"
    "CPU part        : 0x2027\n"
    "CPU revision    : 0\n"
    "\n"
    "processor       : 7\n"
    "BogoMIPS        : 40.00\n"
    "CPU implementer : 0x51\n"
    "CPU architecture: 8\n"
    "CPU variant     : 0x2\n"
    "CPU part        : 0x2027\n"
    "CPU revision    : 0\n"
    "\n"
    "processor       : 8\n"
    "BogoMIPS        : 40.00\n"
    "CPU implementer : 0x51\n"
    "CPU architecture: 8\n"
    "CPU variant     : 0x2\n"
    "CPU part        : 0x2027\n"
    "CPU revision    : 0\n"
    "\n"
    "processor       : 9\n"
    "BogoMIPS        : 40.00\n"
    "CPU implementer : 0x51\n"
    "CPU architecture: 8\n"
    "CPU variant     : 0x2\n"
    "CPU part        : 0x2027\n"
    "CPU revision    : 0\n"
    "\n"
    "Hardware        : Samsung Exynos 2600\n"
    "Revision        : 0001\n"
    "Serial          : 0000000000000000\n";

static int   module_enabled;
static int   tracked_fds[384];

#define TS_TRACK_CPUINFO  0x08000000

static void fd_track_add(int fd) {
    for (int i = 0; i < 384; i++) {
        if (tracked_fds[i] == 0) {
            tracked_fds[i] = fd;
            return;
        }
    }
}

static int fd_is_tracked(int fd) {
    for (int i = 0; i < 384; i++) {
        if (tracked_fds[i] == fd)
            return 1;
    }
    return 0;
}

static void fd_track_remove(int fd) {
    for (int i = 0; i < 384; i++) {
        if (tracked_fds[i] == fd) {
            tracked_fds[i] = 0;
            return;
        }
    }
}

static long before_openat(struct pt_regs *regs, long *ret) {
    if (!module_enabled) return 0;
    char buf[64];
    long r = strncpy_from_user(buf, (const char __user *)regs->regs[1], sizeof(buf) - 1);
    if (r < 0) return 0;
    buf[r] = '\0';
    if (kf_strcmp(buf, "/proc/cpuinfo") == 0 || kf_strcmp(buf, "cpuinfo") == 0) {
        unsigned long *status = (unsigned long *)regs->sp;
        *status |= TS_TRACK_CPUINFO;
    }
    return 0;
}

static long after_openat(struct pt_regs *regs, long *ret) {
    if (!module_enabled) return 0;
    long fd = *ret;
    if (fd < 0) return 0;
    unsigned long *status = (unsigned long *)regs->sp;
    if (*status & TS_TRACK_CPUINFO) {
        *status &= ~TS_TRACK_CPUINFO;
        fd_track_add((int)fd);
    }
    return 0;
}

static long before_close(struct pt_regs *regs, long *ret) {
    if (!module_enabled) return 0;
    int fd = (int)regs->regs[0];
    fd_track_remove(fd);
    return 0;
}

static long before_read(struct pt_regs *regs, long *ret) {
    if (!module_enabled) return 0;
    int fd           = (int)regs->regs[0];
    void __user *buf = (void __user *)regs->regs[1];
    size_t count     = (size_t)regs->regs[2];

    if (!fd_is_tracked(fd)) return 0;

    size_t fake_len = kf_strlen(fake_cpuinfo);
    size_t copy_len = (count < fake_len) ? count : fake_len;

    long err = copy_to_user(buf, fake_cpuinfo, copy_len);
    if (err != 0) return 0;

    *ret = (long)copy_len;
    return 1;
}

static long cpuinfo_ctl0(const char *args, char *event, int dryrun) {
    if (kf_strcmp(args, CONTROL_TOKEN) == 0) {
        module_enabled = 1;
        kf_memset(event, 0, 64); 
        return 0;
    }
    module_enabled = 0;
    return 0;
}

static long cpuinfo_init(const char *args, const char *event, int dryrun) {
    printk(KERN_INFO "cpuinfo Exynos2600 init, event=%s args=%s\n", event, args);
    kf_memset(tracked_fds, 0, sizeof(tracked_fds));
    module_enabled = 0;
    long err;

    err = hook_syscalln(__NR_openat, 2, before_openat, after_openat, NULL);
    if (err) {
        printk(KERN_ERR "cpuinfo hook openat error: %d\n", (int)err);
        return err;
    }
    err = hook_syscalln(__NR_read, 1, before_read, NULL, NULL);
    if (err) {
        printk(KERN_ERR "cpuinfo hook read error: %d\n", (int)err);
        goto err_read;
    }
    err = hook_syscalln(__NR_close, 1, before_close, NULL, NULL);
    if (err) {
        printk(KERN_ERR "cpuinfo hook close error: %d\n", (int)err);
        goto err_close;
    }
    return 0;

err_close:
    unhook_syscalln(__NR_read, before_read, NULL);
err_read:
    unhook_syscalln(__NR_openat, before_openat, after_openat);
    return err;
}

static long cpuinfo_exit(const char *event, int dryrun) {
    printk(KERN_INFO "cpuinfo Exynos2600 exit\n");
    unhook_syscalln(__NR_close,  before_close,  NULL);
    unhook_syscalln(__NR_read,   before_read,   NULL);
    unhook_syscalln(__NR_openat, before_openat, after_openat);
    return 0;
}

KPM_NAME("cpuinfo-exynos2600");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("TG @huanjingnb66");
KPM_DESCRIPTION("CPU info semantics only; input @huanjingnb66 to enable");

KPM_INIT(cpuinfo_init);
KPM_CTL0(cpuinfo_ctl0);
KPM_EXIT(cpuinfo_exit);