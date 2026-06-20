/* kpm.h - 基于二进制逆向分析的正确版本 */
#ifndef _KPM_H_
#define _KPM_H_

#include <stddef.h>

/* KernelPatch 在加载时提供的内核函数 */
extern long   kf_strcmp(const char *cs, const char *ct);
extern size_t kf_strlen(const char *s);
extern void  *kf_memset(void *s, int c, size_t count);
extern void  *kf_memcpy(void *dst, const void *src, size_t count);

/* syscall hook 接口 */
extern long hook_syscalln(long nr, int nargs,
                          void *before, void *after, void *udata);
extern long unhook_syscalln(long nr, void *before, void *after);

/* 模块元信息（.kpm.info 节，格式为 "key=value\0"） */
#define KPM_NAME(_n) \
    __attribute__((section(".kpm.info"), used)) \
    static const char __kpm_name[] = "name=" _n "\0"

#define KPM_VERSION(_v) \
    __attribute__((section(".kpm.info"), used)) \
    static const char __kpm_version[] = "version=" _v "\0"

#define KPM_LICENSE(_l) \
    __attribute__((section(".kpm.info"), used)) \
    static const char __kpm_license[] = "license=" _l "\0"

#define KPM_AUTHOR(_a) \
    __attribute__((section(".kpm.info"), used)) \
    static const char __kpm_author[] = "author=" _a "\0"

#define KPM_DESCRIPTION(_d) \
    __attribute__((section(".kpm.info"), used)) \
    static const char __kpm_description[] = "description=" _d "\0"

/* 模块生命周期函数注册（.kpm.init / .kpm.ctl0 / .kpm.exit 节） */
#define KPM_INIT(_f) \
    __attribute__((section(".kpm.init"), used)) \
    static void *__kpm_init = (void *)(_f)

#define KPM_CTL0(_f) \
    __attribute__((section(".kpm.ctl0"), used)) \
    static void *__kpm_ctl0 = (void *)(_f)

#define KPM_EXIT(_f) \
    __attribute__((section(".kpm.exit"), used)) \
    static void *__kpm_exit = (void *)(_f)

#endif /* _KPM_H_ */
