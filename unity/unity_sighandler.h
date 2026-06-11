// unity_sighandler.h
#ifndef UNITY_SIGHANDLER_H
#define UNITY_SIGHANDLER_H

#include <setjmp.h>
#include "unity.h"

extern jmp_buf segfault_jmp;
extern volatile int in_test;

void unity_install_sighandler(void);

#define RUN_TEST_SAFE(func)                                 \
    do {                                                    \
        in_test = 1;                                        \
        if (setjmp(segfault_jmp) == 0) {                    \
            Unity.CurrentTestFailed = 0;                    \
            UnityDefaultTestRun(func, #func, __LINE__);     \
        } else {                                            \
            UnityPrint("SEGFAULT in " #func "\n");          \
            Unity.TestFailures++;                           \
        }                                                   \
        in_test = 0;                                        \
    } while(0)

#endif