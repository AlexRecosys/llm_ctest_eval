// unity_sighandler.c
#define _POSIX_C_SOURCE 200809L
#include "unity_sighandler.h"
#include <signal.h>
#include <stddef.h>

jmp_buf segfault_jmp;
volatile int in_test = 0;

static void segfault_handler(int sig) {
    (void)sig;
    if (in_test) {
        in_test = 0;
        longjmp(segfault_jmp, 1);
    }
    signal(SIGSEGV, SIG_DFL);
    raise(SIGSEGV);
}

void unity_install_sighandler(void) {
    struct sigaction sa = {0};
    sa.sa_handler = segfault_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGSEGV, &sa, NULL);
}