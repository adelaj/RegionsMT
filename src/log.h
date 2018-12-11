#pragma once

#include "np.h"
#include "common.h"
#include "strproc.h"
#include "utf8.h"

void print(char *, size_t *, const char *, size_t);
bool print_fmt(char *, size_t *, ...);
bool print_time_diff(char *, size_t *, uint64_t, uint64_t);
void print_time_stamp(char *, size_t *);

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

#define ENV_INIT(BEGIN, END) { .begin = STRI(BEGIN), .end = STRI(END) }
#define ENV_INIT_COL_EXT(BEGIN, COL, END) ENV_INIT(ANSI CSI SGR(TOSTRING(COL)) BEGIN, END ANSI CSI SGR(TOSTRING(FG_RESET)))
#define ENV_INIT_COL(COL) ENV_INIT_COL_EXT("", COL, "")

struct env {
    struct strl begin, end;
};

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
    struct env ttl[MESSAGE_CNT], inf, tmd, num, pth, str, chr;
};

struct log {
    FILE *file;
    char *buff;
    struct style style;
    size_t cnt, cap, lim;
    uint64_t tot; // File size is 64-bit always!
};

typedef bool (*message_callback)(char *, size_t *, void *, struct style);
typedef bool (*message_callback_var)(char *, size_t *, void *, struct style, Va_list);

struct code_metric {
    struct strl path, func;
    size_t line;
};

#define CODE_METRIC \
    (struct code_metric) { .path = STRI(__FILE__), .func = STRI(__func__), .line = (__LINE__) }

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
bool log_message_crt(struct log *restrict, struct code_metric, enum message_type, Errno_t);
bool log_message_fopen(struct log *restrict, struct code_metric, enum message_type, const char *restrict, Errno_t);
bool log_message_fseek(struct log *restrict, struct code_metric, enum message_type, int64_t, const char *restrict);
