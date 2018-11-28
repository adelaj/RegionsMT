#include "np.h"
#include "ll.h"
#include "log.h"
#include "memory.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

void print(char *buff, size_t *p_cnt, const char *str, size_t len)
{
    size_t cnt = *p_cnt;
    if (cnt && cnt - 1 >= len) memcpy(buff, str, len + 1);
    *p_cnt = len;
}

bool print_fmt_var(char *buff, size_t *p_cnt, va_list arg)
{
    const char *fmt = va_arg(arg, const char *);
    if (fmt)
    {
        size_t cnt = *p_cnt;
        if (cnt)
        {
            int tmp = vsnprintf(buff, cnt, fmt, arg);
            if (tmp < 0) return 0;
            *p_cnt = (size_t) tmp;
        }
        else *p_cnt = 1; // Requesting string of non-zero length
    }
    else *p_cnt = 0; // Zero bytes written
    return 1;
}

enum fmt_type {
    TYPE_INT = 0,
    TYPE_CHAR,
    TYPE_STR,
    TYPE_FLT,
    TYPE_TIME_DIFF,
    TYPE_TIME_STAMP,
    TYPE_FMT,
    TYPE_PERC,
    TYPE_UTF,
    TYPE_EMPTY
};

enum fmt_int_spec {
    INT_SPEC_DEFAULT = 0,
    INT_SPEC_BYTE,
    INT_SPEC_WORD,
    INT_SPEC_DWORD,
    INT_SPEC_QWORD,
    INT_SPEC_SIZE,
    INT_SPEC_SHRTSHRT, // 'signed char' or 'unsigned char'
    INT_SPEC_SHRT,
    INT_SPEC_LONG,
    INT_SPEC_LONGLONG,
};

enum fmt_int_flags {
    INT_FLAG_UNSIGNED = 1,
    INT_FLAG_HEX = 2,
    INT_FLAG_UPPERCASE = 4
};

enum fmt_flt_spec {
    FLT_SPEC_DEFAULT = 0,
    FLT_SPEC_EXP = 1
};

enum fmt_flt_flags {
    FLT_FLAG_PAYLOAD = 1,
    FLT_FLAG_HEX = 2,
    FLT_FLAG_UPPERCASE = 4
};

enum frm_res_flags {
    FMT_CONDITIONAL = 1,
    FMT_ENV_BEGIN = 2,
    FMT_ENV_END = 4,
};

enum frm_arg_mode {
    ARG_DEFAULT = 0,
    ARG_SET,
    ARG_FETCH,
};

struct fmt_res {
    enum frm_res_flags flags;
    enum fmt_type type;
    union {
        struct {
            enum fmt_int_spec spec;
            enum fmt_int_flags flags;
        } int_arg;
        struct {
            enum frm_arg_mode mode;
            enum fmt_flt_spec spec;
            enum fmt_flt_flags flags;
            size_t prec;
        } flt_arg;
        struct {
            enum frm_arg_mode mode;
            size_t len;
        } str_arg;
        struct {
            enum frm_arg_mode mode;
            uint32_t val;
        } utf_arg;        
    };
};

static bool decode_fmt_int(enum fmt_int_spec *p_spec, enum fmt_int_flags *p_flags, const char *fmt, size_t *p_pos)
{
    size_t pos = *p_pos;
    for (size_t i = 0; i < 2; i++) switch (i)
    {
    case 0:
        switch (fmt[pos])
        {
        case 'b':
            *p_spec = INT_SPEC_BYTE;
            break;
        case 'w':
            *p_spec = INT_SPEC_WORD;
            break;
        case 'd':
            *p_spec = INT_SPEC_DWORD;
            break;
        case 'q':
            *p_spec = INT_SPEC_QWORD;
            break;
        case 'z':
            *p_spec = INT_SPEC_SIZE;
            break;
        case 'h':
            if (fmt[pos + 1] == 'h')
            {
                *p_spec = INT_SPEC_SHRTSHRT;
                pos++;
            }
            else *p_spec = INT_SPEC_SHRT;
            break;
        case 'l':
            if (fmt[pos + 1] == 'l')
            {
                *p_spec = INT_SPEC_LONGLONG;
                pos++;
            }
            else *p_spec = INT_SPEC_LONG;
            break;
        default:
            return 1;
        }
        pos++;
        break;
    case 1:
        if (fmt[pos] != 'x' && fmt[pos] != 'X') break;
        *p_flags |= INT_FLAG_HEX;
        pos++;
        break;
    }
    *p_pos = pos;
    return 1;
}

static bool decode_fmt_size(size_t *p_val, enum frm_arg_mode *p_mode, const char *fmt, size_t *p_pos)
{
    size_t pos = *p_pos;
    if (fmt[pos] == '*')
    {
        *p_mode = ARG_FETCH;
        *p_pos = pos + 1;
        return 1;
    }
    const char *tmp;
    unsigned cvt = str_to_size(fmt + pos, &tmp, p_val);
    if (tmp == fmt + pos) return 1;
    if (!cvt) return 0;
    *p_pos = tmp - fmt;    
    *p_mode = ARG_SET;
    return 1;
}

static bool decode_fmt_str(uint32_t *p_val, enum frm_arg_mode *p_mode, const char *fmt, size_t *p_pos)
{
    bool hex = 0;
    size_t pos = *p_pos;
    if (fmt[pos] == 'x' || fmt[pos] == 'X')
    {
        pos++;
        hex = 1;
    }
    if (fmt[pos] == '*')
    {
        *p_mode = ARG_FETCH;
        *p_pos = pos + 1;
        return 1;
    }
    const char *tmp;
    unsigned cvt = (hex ? str_to_uint32_hex : str_to_uint32)(fmt + pos, &tmp, p_val);
    *p_pos = tmp - fmt;
    if (tmp == fmt + pos) return 1;
    if (!cvt || *p_val >= UTF8_BOUND) return 0;
    *p_mode = ARG_SET;
    return 1;
}

static bool decode_fmt(struct fmt_res *res, const char *fmt, size_t *p_pos)
{
    const char flags[] = { '?', '<', '>' };
    size_t pos = *p_pos;
    *res = (struct fmt_res) { 0 };
    for (size_t i = 0; i < 4; i++) switch (i)
    {
    case 0:
    case 1:
    case 2:
        if (fmt[pos] != flags[i]) break;
        res->flags |= 1 << i;
        pos++;
        break;
    case 3:
        switch (fmt[pos])
        {
        case 'u':
            res->int_arg.flags |= INT_FLAG_UNSIGNED;
        case 'i':
            res->type = TYPE_INT;
            *p_pos = pos;
            return decode_fmt_int(&res->int_arg.flags, &res->int_arg.spec, fmt, p_pos);
        case 'c':
            res->type = TYPE_CHAR;
            break;
        case 's':
            res->type = TYPE_STR;
            *p_pos = pos;
            return decode_fmt_str(&res->str_arg.len, &res->str_arg.mode, fmt, p_pos);
        case 'A':
            res->flt_arg.flags |= FLT_FLAG_UPPERCASE;
        case 'a':
            res->flt_arg.flags |= FLT_FLAG_HEX;
            res->flt_arg.spec = FLT_SPEC_EXP;
        case 'f':
            res->type = TYPE_FLT;
            *p_pos = pos;
            return decode_fmt_str(&res->flt_arg.prec, &res->flt_arg.mode, fmt, p_pos);
        case 'E':
            res->flt_arg.flags |= FLT_FLAG_UPPERCASE;
        case 'e':
            res->flt_arg.spec = FLT_SPEC_EXP;
            res->type = TYPE_FLT;
            *p_pos = pos;
            return decode_fmt_str(&res->flt_arg.prec, &res->flt_arg.mode, fmt, p_pos);
        case 'T':
            res->type = TYPE_TIME_DIFF;
            break;
        case 'D':
            res->type = TYPE_TIME_STAMP;
            break;
        case '$':
            res->type = TYPE_FMT;
            break;
        case '%':
            res->type = TYPE_PERC;
            break;
        case '#':
            res->type = TYPE_UTF;
            *p_pos = pos;
            return decode_fmt_utf(&res->utf_arg.val, &res->utf_arg.mode, fmt, p_pos);
        case ' ':
            res->type = TYPE_EMPTY;
            continue;
        default:
            *p_pos = pos;
            return 0;
        }
    }
    *p_pos = pos;
    return 1;
}

bool execute_fmt(const char *fmt, char *buff, size_t *p_cnt, va_list arg, bool phony)
{
    size_t cnt = *p_cnt;
    for (size_t i = 0, len = 0, tmp = *p_cnt, pos = 0; (tmp = len = size_sub_sat(tmp, len)); cnt = size_add_sat(cnt, len), i = (i + 1) & 1)
    {
        if (!i)
        {
            size_t ctr = Strchrnul(fmt + pos, '%');
            print(buff + cnt, &len, fmt + pos, ctr);
            if (!fmt[pos += ctr]) break;
        }
        else
        {
            size_t off;
            struct fmt_res res;
            if (!decode_fmt(&res, fmt + pos, &off)) return 0;
            pos += off;
            bool no_print = phony;
            //if (res.flags & FMT_CONDITIONAL) phony &= va_arg(arg, bool);
            //if (res.flags & (FMT_ENV_BEGIN | FMT_ENV_END)) 
        }
    }
    *p_cnt = cnt;
}

bool print_fmt2_var(char *buff, size_t *p_cnt, va_list arg)
{
    const char *fmt = va_arg(arg, const char *);
    return !fmt || execute_fmt(fmt, buff, p_cnt, arg, 0);
}

bool print_fmt(char *buff, size_t *p_cnt, ...)
{
    va_list arg;
    va_start(arg, p_cnt);
    bool res = print_fmt_var(buff, p_cnt, arg);
    va_end(arg);
    return res;
}

bool print_time_diff(char *buff, size_t *p_cnt, uint64_t start, uint64_t stop, struct env env)
{
    uint64_t diff = DIST(stop, start), hdq = diff / 3600000000, hdr = diff % 3600000000, mdq = hdr / 60000000, mdr = hdr % 60000000;
    double sec = 1.e-6 * (double) mdr;
    return
        hdq ? print_fmt(buff, p_cnt, "%s%" PRIu64 "%s hr %s%" PRIu64 "%s min %s%.6f%s sec", ENV(env, hdq), ENV(env, mdq), ENV(env, sec)) :
        mdq ? print_fmt(buff, p_cnt, "%s%" PRIu64 "%s min %s%.6f%s sec", ENV(env, mdq), ENV(env, sec)) :
        sec >= 1.e-6 ? print_fmt(buff, p_cnt, "%s%.6f%s sec", ENV(env, sec)) :
        print_fmt(buff, p_cnt, "less than %s%.6f%s sec", ENV(env, 1.e-6));
}

void print_time_stamp(char *buff, size_t *p_cnt)
{
    size_t cnt = *p_cnt;
    if (cnt)
    {
        time_t t;
        time(&t);
        struct tm ts;
        Localtime_s(&ts, &t);
        size_t len = strftime(buff, cnt, "%Y-%m-%d %H:%M:%S UTC%z", &ts);
        *p_cnt = len ? len : size_add_sat(cnt, cnt);
    }
    else *p_cnt = 1;
}

bool message_time_diff(char *buff, size_t *p_cnt, void *Context, struct style style)
{
    struct time_diff *context = Context;
    size_t len = *p_cnt, cnt = 0;
    for (unsigned i = 0;; i++)
    {
        size_t tmp = len;
        switch (i)
        {
        case 0:
            if (!print_time_diff(buff + cnt, &tmp, context->start, context->stop, style.time)) return 0;
            break;
        case 1:
            print(buff + cnt, &tmp, STRC(".\n"));
        }
        cnt = size_add_sat(cnt, tmp);
        if (i == 1) break;
        len = size_sub_sat(len, tmp);
    }
    *p_cnt = cnt;
    return 1;
}

bool message_crt(char *buff, size_t *p_cnt, void *Context, struct style style)
{
    (void) style;
    size_t cnt = *p_cnt;
    if (!Strerror_s(buff, cnt, *(Errno_t *) Context))
    {
        size_t len = Strnlen(buff, cnt); // 'len' is not greater than 'cnt' due to the previous condition
        int tmp = snprintf(buff + len, cnt - len, "!\n");
        if (tmp < 0) return 0;
        *p_cnt = size_add_sat(len, (size_t) tmp);
        return 1; 
    }
    *p_cnt = size_add_sat(cnt, cnt);
    return 1;
}

bool message_var(char *buff, size_t *p_cnt, void *context, struct style style, va_list arg)
{
    (void) style;
    (void) context;
    return print_fmt_var(buff, p_cnt, arg);
}

bool message_var_two_stage(char *buff, size_t *p_cnt, void *Thunk, struct style style, va_list arg)
{
    size_t cnt = 0;
    struct message_thunk *thunk = Thunk;
    for (size_t i = 0, len = 0, tmp = *p_cnt; i < 2 && (tmp = len = size_sub_sat(tmp, len)); cnt = size_add_sat(cnt, len), i++) switch (i)
    {
    case 0:
        if (!print_fmt_var(buff + cnt, &len, arg)) return 0;
        break;
    case 1:
        if (!thunk->handler(buff + cnt, &len, thunk->context, style)) return 0;
        break;
    }
    *p_cnt = cnt;
    return 1;
}

// Last argument may be 'NULL'
bool log_init(struct log *restrict log, char *restrict path, size_t lim, enum log_flags flags, struct style style, struct log *restrict log_error)
{
    FILE *f = path ? fopen(path, flags & LOG_APPEND ? "a" : "w") : stderr;
    if (path && !f) log_message_fopen(log_error, CODE_METRIC, MESSAGE_ERROR, path, errno);
    else
    {
        bool tty = file_is_tty(f), bom = !(tty || (flags & (LOG_NO_BOM | LOG_APPEND)));
        size_t cap = bom ? MAX(lim, lengthof(UTF8_BOM)) : lim;
        if (!array_init(&log->buff, &log->cap, cap, sizeof(*log->buff), 0, 0)) log_message_crt(log_error, CODE_METRIC, MESSAGE_ERROR, errno);
        else
        {
            if (bom) memcpy(log->buff, UTF8_BOM, log->cnt = lengthof(UTF8_BOM));
            else log->cnt = 0;
            log->style = tty ? style : (struct style) { 0 };
            log->lim = lim;
            log->file = f;
            log->tot = 0;
            if (log->cnt < log->lim || log_flush(log)) return 1;
            free(log->buff);
        }
        Fclose(f);
    }
    return 0;
}

bool log_flush(struct log *restrict log)
{
    size_t cnt = log->cnt;
    if (cnt)
    {
        size_t wr = fwrite(log->buff, 1, cnt, log->file);
        log->tot += wr;
        log->cnt = 0;
        fflush(log->file);
        return wr == cnt;
    }
    return 1;
}

// May be used for 'log' allocated by 'calloc' (or filled with zeros statically)
void log_close(struct log *restrict log)
{
    log_flush(log);
    Fclose(log->file);
    free(log->buff);
}

bool log_multiple_init(struct log *restrict arr, size_t cnt, char *restrict path, size_t cap, enum log_flags flags, struct style style, struct log *restrict log_error)
{
    size_t i = 0;
    for (; i < cnt && log_init(arr + i, path, cap, flags | LOG_APPEND, style, log_error); i++);
    if (i == cnt) return 1;
    for (; --i; log_close(arr + i));
    return 0;
}

void log_multiple_close(struct log *restrict arr, size_t cnt)
{
    if (cnt) for (size_t i = cnt; --i; log_close(arr + i));
}

static bool log_prefix(char *buff, size_t *p_cnt, struct code_metric code_metric, enum message_type type, struct style style)
{
    const struct strl ttl[] = { STRI("MESSAGE"), STRI("ERROR"), STRI("WARNING"), STRI("NOTE"), STRI("INFO"), STRI("DAMNATION") };
    size_t cnt = 0;
    if (!print_fmt2(buff + cnt, p_cnt, "%<>D %<>s* %< %<>s* @ %<>s*:%uz%>  ", style.ts, style.ttl[type], ttl[type].str, ttl[type].len, ENV(style.src, ENV(style.dquo, code_metric.func), ENV(style.dquo, code_metric.path), code_metric.line))) return 0;
    *p_cnt = cnt;
    return 1;
}

static bool log_message_impl(struct log *restrict log, struct code_metric code_metric, enum message_type type, message_callback handler, message_callback_var handler_var, void *context, va_list arg)
{
    if (!log) return 1;
    size_t cnt = log->cnt;
    for (unsigned i = 0; i < 2; i++) for (;;)
    {
        size_t len = log->cap - cnt;
        switch (i)
        {
        case 0:
            if (!log_prefix(log->buff + cnt, &len, code_metric, type, log->style)) return 0;
            break;
        case 1:
            if (handler_var)
            {
                va_list arg2;
                va_copy(arg2, arg);
                bool res = handler_var(log->buff + cnt, &len, context, log->style, arg2);
                va_end(arg2);
                if (!res) return 0;
            }
            else if (handler && !handler(log->buff + cnt, &len, context, log->style)) return 0;
        }
        unsigned res = array_test(&log->buff, &log->cap, sizeof(*log->buff), 0, 0, cnt, len, 1); // All messages are NULL-terminated
        if (!res) return 0;
        if (!(res & ARRAY_UNTOUCHED)) continue;
        cnt += len;
        break;
    }
    log->cnt = cnt;
    return cnt >= log->lim ? log_flush(log) : 1;
}

// Emulation of passing '(va_list) 0'
static bool log_message_supp(struct log *restrict log, struct code_metric code_metric, enum message_type type, message_callback handler, void *context, ...)
{
    va_list arg;
    va_start(arg, context);
    bool res = log_message_impl(log, code_metric, type, handler, NULL, context, arg);
    va_end(arg);
    return res;
}

bool log_message(struct log *restrict log, struct code_metric code_metric, enum message_type type, message_callback handler, void *context)
{
    return log_message_supp(log, code_metric, type, handler, context);
}

bool log_message_var(struct log *restrict log, struct code_metric code_metric, enum message_type type, message_callback_var handler_var, void *context, ...)
{
    va_list arg;
    va_start(arg, context);
    bool res = log_message_impl(log, code_metric, type, NULL, handler_var, context, arg);
    va_end(arg);
    return res;
}

bool log_message_fmt(struct log *restrict log, struct code_metric code_metric, enum message_type type, ...)
{
    va_list arg;
    va_start(arg, type);
    bool res = log_message_impl(log, code_metric, type, NULL, message_var, NULL, arg);
    va_end(arg);
    return res;
}

bool log_message_time_diff(struct log *restrict log, struct code_metric code_metric, enum message_type type, uint64_t start, uint64_t stop, ...)
{
    va_list arg;
    va_start(arg, stop);
    bool res = log_message_impl(log, code_metric, type, NULL, message_var_two_stage, &(struct message_thunk) { .handler = message_time_diff, .context = &(struct time_diff) { .start = start, .stop = stop } }, arg);
    va_end(arg);
    return res;
}

bool log_message_crt(struct log *restrict log, struct code_metric code_metric, enum message_type type, Errno_t err)
{
    return log_message(log, code_metric, type, message_crt, &err);
}

bool log_message_fopen(struct log *restrict log, struct code_metric code_metric, enum message_type type, const char *restrict path, Errno_t err)
{
    return log_message_var(log, code_metric, type, message_var_two_stage, &(struct message_thunk) { .handler = message_crt, .context = &err }, "Unable to open the file %s%s%s%s%s: ", ENV(log->style.path, ENV(log->style.dquo, path)));
}

bool log_message_fseek(struct log *restrict log, struct code_metric code_metric, enum message_type type, int64_t offset, const char *restrict path)
{
    return log_message_fmt(log, code_metric, type, "Unable to seek into the position %s%" PRId64 "%s while reading the file %s%s%s%s%s!\n", ENV(log->style.num, offset), ENV(log->style.path, ENV(log->style.dquo, path)));
}
