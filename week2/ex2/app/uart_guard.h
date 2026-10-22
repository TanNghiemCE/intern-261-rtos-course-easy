#ifndef UART_GUARD_H
#define UART_GUARD_H

#include <stddef.h>

/* 0 = none (the bug), 1 = mutex, 2 = critical section, 3 = gatekeeper task.
 * Chon mode tuong ung de bien dich va kiem tra. */
#ifndef UART_GUARD_MODE
#define UART_GUARD_MODE 2
#endif

/* 1 = Bat do do tre DWT va xuat bang bao cao tong ket duy nhat
 * 0 = Che do in lien tuc thong thuong cua cac task writer ra UART */
#ifndef ENABLE_LATENCY_REPORT
#define ENABLE_LATENCY_REPORT 1
#endif

void uart_guard_init(void);

/* Blocking: sends `len` bytes out USART1 under whatever UART_GUARD_MODE picks.
 * In gatekeeper mode this enqueues to the gatekeeper instead of transmitting
 * directly — same call site either way. */
void uart_guard_send(const char *buf, size_t len);

#if UART_GUARD_MODE == 3
void gatekeeper_task(void *argument);
#endif

#endif /* UART_GUARD_H */
