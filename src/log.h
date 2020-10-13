#pragma once

#include "np.h"
#include "common.h"
#include "strproc.h"
#include "utf8.h"

typedef Errno_t Errno_dom_t;

/*
#define ARG_PTR(ARG, PTR) \
    (sizeof(*(PTR)) <= sizeof(void *) && alignof(*(PTR)) <= alignof(void *) ? (void *) (ARG) : (void *) (PTR))

#define ARGI(T, ARG) \
    ARG_PTR((ARG), &(T) { (ARG) })

#define DECL_ARG_FETCH(PREFIX, TYPE, TYPE_DOM) \
    TYPE PREFIX ## _arg_fetch(void *p_arg, bool ptr) \
    { \
        return ptr ? \
            sizeof(TYPE) <= sizeof(void *) && alignof(TYPE) <= alignof(void *) ? (TYPE) *(*(void ***) p_arg)++ : *(TYPE *) *(*(void ***) p_arg)++ : \
            (TYPE) Va_arg(*(Va_list *) p_arg, TYPE_DOM); \
    }

#define DECL_STRUCT_ARG_FETCH(PREFIX, TYPE) \
    TYPE PREFIX ## _arg_fetch(void *p_arg, bool ptr) \
    { \
        return ptr ? *(TYPE *) *(*(void ***) p_arg)++ : Va_arg(*(Va_list *) p_arg, TYPE); \
    }
    */

#define DECL_PTR_ARG_FETCH(PREFIX, TYPE) \
    TYPE *PREFIX ## _arg_fetch(void *p_arg, bool ptr) \
    { \
        return ptr ? (TYPE *) *(*(void ***) p_arg)++ : Va_arg(*(Va_list *) p_arg, TYPE *); \
    }

#define DECL_ARG_FETCH(PREFIX, TYPE, TYPE_DOM) \
    TYPE PREFIX ## _arg_fetch(void *p_arg, bool ptr) \
    { \
        return ptr ? *(TYPE *) *(*(void ***) p_arg)++ : (TYPE) Va_arg(*(Va_list *) p_arg, TYPE_DOM); \
    }

#define DECL_STRUCT_ARG_FETCH(PREFIX, TYPE) \
    TYPE PREFIX ## _arg_fetch(void *p_arg, bool ptr) \
    { \
        return ptr ? *(TYPE *) *(*(void ***) p_arg)++ : Va_arg(*(Va_list *) p_arg, TYPE); \
    }

void *ptr_arg_fetch(void *p, bool);
struct env *env_ptr_arg_fetch(void *p, bool);
struct style *style_ptr_arg_fetch(void *p, bool);

unsigned char ussint_arg_fetch(void *p, bool);
unsigned short usint_arg_fetch(void *p, bool);
unsigned uint_arg_fetch(void *p, bool);
unsigned long ulint_arg_fetch(void *p, bool);
unsigned long long ullint_arg_fetch(void *p, bool);
uint8_t uint8_arg_fetch(void *p, bool);
uint16_t uint16_arg_fetch(void *p, bool);
uint32_t uint32_arg_fetch(void *p, bool);
uint64_t uint64_arg_fetch(void *p, bool);
size_t size_arg_fetch(void *p, bool);
uintmax_t uintmax_arg_fetch(void *p, bool);

bool bool_arg_fetch(void *p, bool);
Errno_t Errno_arg_fetch(void *p, bool);
signed char ssint_arg_fetch(void *p, bool);
short sint_arg_fetch(void *p, bool);
int int_arg_fetch(void *p, bool);
long lint_arg_fetch(void *p, bool);
long long llint_arg_fetch(void *p, bool);
int8_t int8_arg_fetch(void *p, bool);
int16_t int16_arg_fetch(void *p, bool);
int32_t int32_arg_fetch(void *p, bool);
int64_t int64_arg_fetch(void *p, bool);
ptrdiff_t ptrdiff_arg_fetch(void *p, bool);
intmax_t intmax_arg_fetch(void *p, bool);

enum fmt_execute_flags {
    FMT_EXE_FLAG_PHONY = 1,
    FMT_EXE_FLAG_PTR = 2
};

#define ESC "\x1b"
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

#define ENVI(BEGIN, END) { .begin = STRI(BEGIN), .end = STRI(END) }
#define ENVI_FG_EXT(BEGIN, COL, END) ENVI(ESC CSI SGR(TOSTR(COL)) BEGIN, END ESC CSI SGR(TOSTR(FG_RESET)))
#define ENVI_FG(COL) ENVI_FG_EXT("", COL, "")

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
    struct env type_int, type_char, type_path, type_str, type_flt, type_time_diff, type_time_stamp, type_code_metric, type_utf;
};

struct ttl_style {
    struct env time_stamp, header[MESSAGE_CNT], code_metric;
};

struct code_metric {
    struct strl path, func;
    size_t line;
};

struct message_result {
    unsigned status;
    struct code_metric metric;
};

#define CODE_METRIC \
    (struct code_metric) { .path = STRI(__FILE__), .func = STRI(__func__), .line = (__LINE__) }

#define MESSAGE_SUCCESS (struct message_result) { .status = 1 }
#define MESSAGE_FAILURE (struct message_result) { .status = 0, .metric = CODE_METRIC }

typedef struct message_result (*fmt_callback)(char *, size_t *, void *, const struct env *, enum fmt_execute_flags);

void print(char *restrict, size_t *restrict, const char *restrict, size_t, bool);
struct message_result print_fmt(char *, size_t *, const struct style *, ...);
struct message_result print_time_diff(char *, size_t *, uint64_t, uint64_t, const struct env *);
void print_time_stamp(char *, size_t *);

struct log {
    FILE *file;
    char *buff;
    const struct ttl_style *ttl_style;
    const struct style *style;
    struct log *fallback; // When fallback is null lock applies to the log buffer also!
    size_t cnt, cap, hint;
    uint64_t tot; // File size is 64-bit always!
    struct mutex *mutex;
};

typedef struct message_result (*message_callback)(char *, size_t *, void *, const struct style *);
typedef struct message_result (*message_callback_var)(char *, size_t *, void *, const struct style *, Va_list);

union message_callback {
    message_callback ord;
    message_callback_var var;
};

enum log_flags {
    LOG_APPEND = 1,
    LOG_NO_BOM = 2,
    LOG_FORCE_TTY = 4,
    LOG_WRITE_LOCK = 8
};

bool log_init(struct log *restrict, const char *restrict, size_t, enum log_flags, const struct ttl_style *restrict, const struct style *restrict, struct log *restrict);
bool log_mirror_init(struct log *restrict, struct log *restrict);
void log_close(struct log *restrict);
void log_mirror_close(struct log *restrict);
bool log_flush(struct log *restrict, bool);

bool log_message(struct log *restrict, struct code_metric, enum message_type, message_callback, void *);
bool log_message_var(struct log *restrict, struct code_metric, enum message_type, message_callback_var, void *, ...);

// Some specialized messages
bool log_message_fmt(struct log *restrict, struct code_metric, enum message_type, ...);
bool log_message_crt(struct log *restrict, struct code_metric, enum message_type, Errno_t);
bool log_message_fopen(struct log *restrict, struct code_metric, enum message_type, const char *restrict, Errno_t);
bool log_message_fseek(struct log *restrict, struct code_metric, enum message_type, int64_t, const char *restrict);

bool fopen_assert(struct log *restrict, struct code_metric, const char *restrict, bool);
bool array_assert(struct log *, struct code_metric, struct array_result);
bool crt_assert_impl(struct log *, struct code_metric, Errno_t);
bool crt_assert(struct log *, struct code_metric, bool);
bool wapi_assert(struct log *, struct code_metric, bool);
