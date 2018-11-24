#pragma once

#include "np.h"
#include "common.h"
#include "strproc.h"
#include "utf8.h"

#include <stdarg.h>

#define ANSI "\x1b"
#define CSI "["
#define SGR(C) C "m"

#define RESET 0
#define FG_BLACK 30
#define FG_RED 31
#define FG_GREEN 32
#define FG_YELLOW 33
#define FG_BLUE 34
#define FG_MAGENTA 35
#define FG_CYAN 36
#define FG_WHITE 37
#define FG_RESET 39
#define FG_BR_BLACK 90
#define FG_BR_RED 91
#define FG_BR_GREEN 92
#define FG_BR_YELLOW 93
#define FG_BR_BLUE 94
#define FG_BR_MAGENTA 95
#define FG_BR_CYAN 96
#define FG_BR_WHITE 97

#define INIT_ENV_COL(COL) \
    { .begin = STRI(ANSI CSI SGR(TOSTRING(COL))), .end = STRI(ANSI CSI SGR(TOSTRING(FG_RESET))) }
#define INIT_ENV_SQUO \
    { .begin = STRI(UTF8_LSQUO), .end = STRI(UTF8_RSQUO) }
#define INIT_ENV_DQUO \
    { .begin = STRI(UTF8_LDQUO), .end = STRI(UTF8_RDQUO) }

#define ENV_GUARD(S) (S).str, (S).len
#define ENV(E, ...) ENV_GUARD((E).begin), __VA_ARGS__, ENV_GUARD((E).end)
#define ENV_MUX_GUARD(S, T, SW) (S ? (E).str : (F).str), (S ? (E).len : (F).len)
#define ENV_MUX(E, F, SW, ...) ENV_MUX_GUARD((E).begin, (F).begin, SW), __VA_ARGS__, ENV_MUX_GUARD((E).end, (F).end, SW)

struct env {
    struct strl begin, end;
};

void print(char *, size_t *, const char *, size_t);
bool print_fmt_var(char *, size_t *, va_list);
bool print_fmt(char *, size_t *, ...);
bool print_time_diff(char *, size_t *, uint64_t, uint64_t, struct env);
void print_time_stamp(char *, size_t *);

enum message_type {
    MESSAGE_DEFAULT = 0,
    MESSAGE_ERROR,
    MESSAGE_WARNING,
    MESSAGE_NOTE,
    MESSAGE_INFO,
    MESSAGE_RESERVED,
    MESSAGE_CNT
};

struct style {
    struct env ts, ttl[MESSAGE_CNT], src, dquo, squo, num, path, str, time;
};

struct log {
    FILE *file;
    char *buff;
    struct style style;
    size_t cnt, cap, lim;
    uint64_t tot; // File size is 64-bit always!
};

typedef bool (*message_callback)(char *, size_t *, void *, struct style);
typedef bool (*message_callback_var)(char *, size_t *, void *, struct style, va_list);

struct code_metric {
    const char *path, *func;
    size_t line;
};

#define CODE_METRIC \
    (struct code_metric) { .path = (__FILE__), .func = (__func__), .line = (__LINE__) }

struct time_diff {
    uint64_t start, stop;
};

bool message_time_diff(char *, size_t *, void *, struct style);
bool message_crt(char *, size_t *, void *, struct style);
bool message_var(char *, size_t *, void *, struct style, va_list);

struct message_thunk {
    message_callback handler;
    void *context;
};

bool message_var_two_stage(char *, size_t *, void *Thunk, struct style, va_list);

#define INTP(X) ((int) MIN((X), INT_MAX)) // Useful for the printf-like functions

enum log_flags {
    LOG_APPEND = 1,
    LOG_NO_BOM = 2
};

bool log_init(struct log *restrict, char *restrict, size_t, enum log_flags, struct style, struct log *restrict);
void log_close(struct log *restrict);
bool log_multiple_init(struct log *restrict, size_t, char *restrict, size_t, enum log_flags, struct style, struct log *restrict);
void log_multiple_close(struct log *restrict, size_t);
bool log_flush(struct log *restrict);
bool log_message(struct log *restrict, struct code_metric, enum message_type, message_callback, void *);
bool log_message_var(struct log *restrict, struct code_metric, enum message_type, message_callback_var, void *, ...);

// Some specialized messages
bool log_message_fmt(struct log *restrict, struct code_metric, enum message_type, ...);
bool log_message_time_diff(struct log *restrict, struct code_metric, enum message_type, uint64_t, uint64_t, ...);
bool log_message_crt(struct log *restrict, struct code_metric, enum message_type, Errno_t);
bool log_message_fopen(struct log *restrict, struct code_metric, enum message_type, const char *restrict, Errno_t);
bool log_message_fseek(struct log *restrict, struct code_metric, enum message_type, int64_t, const char *restrict);
