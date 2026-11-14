/*
 * Calls into AmigaOS libraries: base in a6, arguments in the registers the
 * library's .fd file gives, result in d0.  d0/d1/a0/a1 are scratch across
 * the call and are declared as outputs so the compiler never assumes they
 * survive.  Needs -fomit-frame-pointer, as a6 is otherwise the frame
 * pointer.
 */
#ifndef AMIBOOTNG_LVO_H
#define AMIBOOTNG_LVO_H

#define LVO_SETUP(base)                                                 \
    register long _d0 __asm__("d0");                                    \
    register long _d1 __asm__("d1");                                    \
    register void *_a0 __asm__("a0");                                   \
    register void *_a1 __asm__("a1");                                   \
    register void *_a6 __asm__("a6") = (base)

#define LVO_JSR(lvo, ...)                                               \
    __asm__ volatile("jsr %c4(%%a6)"                                    \
                     : "=r"(_d0), "=r"(_d1), "=r"(_a0), "=r"(_a1)       \
                     : "i"(lvo), "r"(_a6), ##__VA_ARGS__                \
                     : "cc", "memory")

#define LP0(lvo, base) ({                                                \
    LVO_SETUP(base);                                                    \
    LVO_JSR(lvo);                                                       \
    _d0; })

/*
 * The arguments are evaluated before any register variable is bound, so
 * an argument that is itself a function call cannot clobber them.
 */
#define LP1(lvo, base, t1, v1, r1) ({                                   \
    t1 _v1 = (v1);                                                      \
    LVO_SETUP(base);                                                    \
    register t1 _r1 __asm__(#r1) = _v1;                                 \
    LVO_JSR(lvo, "r"(_r1));                                             \
    _d0; })

#define LP2(lvo, base, t1, v1, r1, t2, v2, r2) ({                       \
    t1 _v1 = (v1); t2 _v2 = (v2);                                       \
    LVO_SETUP(base);                                                    \
    register t1 _r1 __asm__(#r1) = _v1;                                 \
    register t2 _r2 __asm__(#r2) = _v2;                                 \
    LVO_JSR(lvo, "r"(_r1), "r"(_r2));                                   \
    _d0; })

#define LP3(lvo, base, t1, v1, r1, t2, v2, r2, t3, v3, r3) ({           \
    t1 _v1 = (v1); t2 _v2 = (v2); t3 _v3 = (v3);                        \
    LVO_SETUP(base);                                                    \
    register t1 _r1 __asm__(#r1) = _v1;                                 \
    register t2 _r2 __asm__(#r2) = _v2;                                 \
    register t3 _r3 __asm__(#r3) = _v3;                                 \
    LVO_JSR(lvo, "r"(_r1), "r"(_r2), "r"(_r3));                         \
    _d0; })

#define LP4(lvo, base, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4) ({ \
    t1 _v1 = (v1); t2 _v2 = (v2); t3 _v3 = (v3); t4 _v4 = (v4);         \
    LVO_SETUP(base);                                                    \
    register t1 _r1 __asm__(#r1) = _v1;                                 \
    register t2 _r2 __asm__(#r2) = _v2;                                 \
    register t3 _r3 __asm__(#r3) = _v3;                                 \
    register t4 _r4 __asm__(#r4) = _v4;                                 \
    LVO_JSR(lvo, "r"(_r1), "r"(_r2), "r"(_r3), "r"(_r4));               \
    _d0; })

#define LP5(lvo, base, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4,  \
            t5, v5, r5) ({                                              \
    t1 _v1 = (v1); t2 _v2 = (v2); t3 _v3 = (v3); t4 _v4 = (v4);         \
    t5 _v5 = (v5);                                                      \
    LVO_SETUP(base);                                                    \
    register t1 _r1 __asm__(#r1) = _v1;                                 \
    register t2 _r2 __asm__(#r2) = _v2;                                 \
    register t3 _r3 __asm__(#r3) = _v3;                                 \
    register t4 _r4 __asm__(#r4) = _v4;                                 \
    register t5 _r5 __asm__(#r5) = _v5;                                 \
    LVO_JSR(lvo, "r"(_r1), "r"(_r2), "r"(_r3), "r"(_r4), "r"(_r5));     \
    _d0; })

#endif
