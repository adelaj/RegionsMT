#include "np.h"
#include "ll.h"
#include "log.h"
#include "memory.h"
#include "intdom.h"

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ARG_FETCH(FLAG, ARG, TYPE, TYPE_DOM) \
    ((FLAG) ? *(TYPE *) *(*(void ***) (ARG))++ : Va_arg(*(Va_list *) (ARG), TYPE_DOM))

static void print_unterminated(char *buff, size_t *p_cnt, const char *str, size_t len)
{
    size_t cnt = *p_cnt;
    *p_cnt = len;
    if (cnt < len) return;
    memcpy(buff, str, len * sizeof(*buff));
}

void print(char *buff, size_t *p_cnt, const char *str, size_t len)
{
    size_t cnt = *p_cnt;
    *p_cnt = len;
    if (!cnt || cnt - 1 < len) return;
    memcpy(buff, str, len * sizeof(*buff));
    buff[len] = '\0';
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

static bool fmt_execute_time_diff_sec(char *buff, size_t *p_cnt, void *p_arg, enum fmt_execute_flags flags)
{
    uint32_t sdr = ARG_FETCH(flags & FMT_EXE_FLAG_PTR, p_arg, uint32_t, Uint32_dom_t);
    if (flags & FMT_EXE_FLAG_PHONY) return 1;
    size_t dig = 0;
    for (uint32_t i = sdr; !(i % 10); dig++, i /= 10);
    return print_fmt(buff, p_cnt, ".000000%-ud%!-*", sdr, dig);    
}

bool print_time_diff(char *buff, size_t *p_cnt, uint64_t start, uint64_t stop)
{
    uint64_t diff = DIST(stop, start), hdq = diff / 3600000000;
    uint32_t hdr = (uint32_t) (diff % 3600000000), mdq = hdr / 60000000, mdr = hdr % 60000000, sdq = mdr / 1000000, sdr = mdr % 1000000;
    return print_fmt(buff, p_cnt, "%?$%?$%ud%?& sec", !!hdq, "%uq hr ", hdq, !!mdq, "%ud min ", mdq, sdq, !!sdr, fmt_execute_time_diff_sec, sdr);
}

void print_crt(char *buff, size_t *p_cnt, Errno_t err)
{
    size_t cnt = *p_cnt;
    *p_cnt = cnt ? Strerror_s(buff, cnt, err) ? size_add_sat(cnt, cnt) : Strnlen(buff, cnt) : 1;
}

enum fmt_type {
    TYPE_DEFAULT = 0,
    TYPE_INT,
    TYPE_CHAR,
    TYPE_STR,
    TYPE_FLT,
    TYPE_TIME_DIFF,
    TYPE_TIME_STAMP,
    TYPE_CRT,
    TYPE_FMT,
    TYPE_PERC,
    TYPE_UTF,
    TYPE_CALLBACK
};

enum fmt_int_spec {
    INT_SPEC_DEFAULT = 0,
    INT_SPEC_BYTE,
    INT_SPEC_WORD,
    INT_SPEC_DWORD,
    INT_SPEC_QWORD,
    INT_SPEC_SIZE,
    INT_SPEC_MAX,
    INT_SPEC_SSHRT, // 'signed char' or 'unsigned char'
    INT_SPEC_SHRT,
    INT_SPEC_LONG,
    INT_SPEC_LLONG,
};

enum fmt_int_flags {
    INT_FLAG_UNSIGNED = 1,
    INT_FLAG_HEX = 2,
    INT_FLAG_UPPERCASE = 4,
};

enum fmt_flt_spec {
    FLT_SPEC_DEFAULT = 0,
    FLT_SPEC_EXP = 1
};

enum fmt_flt_flags {
    FLT_FLAG_HEX = 1,
    FLT_FLAG_UPPERCASE = 2
};

enum fmt_flags {
    FMT_CONDITIONAL = 1,
    FMT_OVERPRINT =  2,
    FMT_ENV_BEGIN = 4,
    FMT_ENV_END = 8,
    FMT_LEFT_JUSTIFY = 16,
    FMT_PTR = 32
};

enum fmt_arg_mode {
    ARG_DEFAULT = 0,
    ARG_SET,
    ARG_FETCH,
};

struct fmt_res {
    enum fmt_flags flags;
    enum fmt_type type;
    size_t wd;
    union {
        struct {
            enum fmt_int_spec spec;
            enum fmt_int_flags flags;
        } int_arg;
        struct {
            enum fmt_arg_mode mode;
            enum fmt_flt_spec spec;
            enum fmt_flt_flags flags;
            size_t prec;
        } flt_arg;
        struct {
            enum fmt_arg_mode mode;
            size_t len;
        } str_arg;
        struct {
            enum fmt_arg_mode mode;
            size_t rep;
        } char_arg;
        struct {
            enum fmt_arg_mode mode;
            uint32_t val;
        } utf_arg;
    };
};

static bool fmt_decode_int(enum fmt_int_spec *p_spec, enum fmt_int_flags *p_flags, const char *fmt, size_t *p_pos)
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
        case 'j':
            *p_spec = INT_SPEC_MAX;
            break;
        case 'h':
            if (fmt[pos + 1] == 'h')
            {
                *p_spec = INT_SPEC_SSHRT;
                pos++;
            }
            else *p_spec = INT_SPEC_SHRT;
            break;
        case 'l':
            if (fmt[pos + 1] == 'l')
            {
                *p_spec = INT_SPEC_LLONG;
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

static bool fmt_decode_str(size_t *p_cnt, enum fmt_arg_mode *p_mode, const char *fmt, size_t *p_pos)
{
    size_t pos = *p_pos;
    if (fmt[pos] == '*')
    {
        *p_mode = ARG_FETCH;
        *p_pos = pos + 1;
        return 1;
    }
    const char *tmp;
    unsigned cvt = str_to_size(fmt + pos, &tmp, p_cnt);
    if (tmp == fmt + pos) return 1;
    *p_pos = tmp - fmt;
    if (!cvt) return 0;
    *p_mode = ARG_SET;
    return 1;
}

static bool fmt_decode_utf(uint32_t *p_val, enum fmt_arg_mode *p_mode, const char *fmt, size_t *p_pos)
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
    if (tmp == fmt + pos || !cvt || *p_val >= UTF8_BOUND) return 0;
    *p_mode = ARG_SET;
    return 1;
}

static bool fmt_decode(struct fmt_res *res, const char *fmt, size_t *p_pos)
{
    size_t pos = *p_pos;
    memset(res, 0, sizeof(*res));
    for (;; pos++)
    {
        enum fmt_flags tmp = 0;
        switch (fmt[pos])
        {
        case '?':
            tmp = FMT_CONDITIONAL;
            break;
        case '!':
            tmp = FMT_OVERPRINT;
            break;        
        case '<':
            tmp = FMT_ENV_BEGIN;
            break;
        case '>':
            tmp = FMT_ENV_END;
            break;
        case '-':
            tmp = FMT_LEFT_JUSTIFY;
            break;
        case '@':
            tmp = FMT_PTR;
            break;
        }
        if (!tmp) break;
        if (res->flags >= tmp) return 1;
        res->flags |= tmp;
    }
    switch (fmt[pos])
    {
    case 'u':
        res->int_arg.flags |= INT_FLAG_UNSIGNED;
    case 'i':
        res->type = TYPE_INT;
        *p_pos = pos + 1;
        return fmt_decode_int(&res->int_arg.spec, &res->int_arg.flags, fmt, p_pos);
    case 'c':
        res->type = TYPE_CHAR;
        *p_pos = pos + 1;
        return fmt_decode_str(&res->char_arg.rep, &res->char_arg.mode, fmt, p_pos);
    case 's':
        res->type = TYPE_STR;
        *p_pos = pos + 1;
        return fmt_decode_str(&res->str_arg.len, &res->str_arg.mode, fmt, p_pos);
    case 'A':
        res->flt_arg.flags |= FLT_FLAG_UPPERCASE;
    case 'a':
        res->flt_arg.flags |= FLT_FLAG_HEX;
        res->flt_arg.spec = FLT_SPEC_EXP;
    case 'f':
        res->type = TYPE_FLT;
        *p_pos = pos + 1;
        return fmt_decode_str(&res->flt_arg.prec, &res->flt_arg.mode, fmt, p_pos);
    case 'E':
        res->flt_arg.flags |= FLT_FLAG_UPPERCASE;
    case 'e':
        res->flt_arg.spec = FLT_SPEC_EXP;
        res->type = TYPE_FLT;
        *p_pos = pos + 1;
        return fmt_decode_str(&res->flt_arg.prec, &res->flt_arg.mode, fmt, p_pos);
    case 'T':
        res->type = TYPE_TIME_DIFF;
        break;
    case 'D':
        res->type = TYPE_TIME_STAMP;
        break;
    case 'C':
        res->type = TYPE_CRT;
        break;
    case '$':
        res->type = TYPE_FMT;
        break;
    case '%':
        res->type = TYPE_PERC;
        break;
    case '#':
        res->type = TYPE_UTF;
        *p_pos = pos + 1;
        return fmt_decode_utf(&res->utf_arg.val, &res->utf_arg.mode, fmt, p_pos);
    case '&':
        res->type = TYPE_CALLBACK;
        break;
    default:
        *p_pos = pos;
        return fmt_decode_str(&res->str_arg.len, &res->str_arg.mode, fmt, p_pos);
    }
    *p_pos = pos + 1;
    return 1;
}

static bool fmt_execute_int(enum fmt_int_spec int_spec, enum fmt_int_flags int_flags, char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    bool u = int_flags & INT_FLAG_UNSIGNED, ptr = flags & FMT_EXE_FLAG_PTR;
    union {
        int i;
        int8_t ib;
        int16_t iw;
        int32_t id;
        int64_t iq;
        ptrdiff_t iz;
        intmax_t ij;
        signed char ihh;
        short ih;
        long il;
        long long ill;        
        unsigned u;
        uint8_t ub;
        uint16_t uw;
        uint32_t ud;
        uint64_t uq;
        size_t uz;
        uintmax_t uj;
        unsigned char uhh;
        unsigned short uh;
        unsigned long ul;
        unsigned long long ull;
    } val = { .uj = 0 };
    switch (int_spec)
    {
    /*
    case INT_SPEC_DEFAULT:
        if (u) val.u = Va_arg(*p_arg, unsigned);
        else val.i = Va_arg(*p_arg, int);
        break;
    case INT_SPEC_BYTE:
        if (u) val.ub = (uint8_t) Va_arg(*p_arg, Uint8_dom_t);
        else val.ib = (int8_t) Va_arg(*p_arg, Int8_dom_t);
        break;
    case INT_SPEC_WORD:
        if (u) val.uw = (uint16_t) Va_arg(*p_arg, Uint16_dom_t);
        else val.iw = (int16_t) Va_arg(*p_arg, Int16_dom_t);
        break;
    case INT_SPEC_DWORD:
        if (u) val.ud = (uint32_t) Va_arg(*p_arg, Uint32_dom_t);
        else val.id = (int32_t) Va_arg(*p_arg, Int32_dom_t);
        break;
    case INT_SPEC_QWORD:
        if (u) val.uq = (uint64_t) Va_arg(*p_arg, Uint64_dom_t);
        else val.iq = (int64_t) Va_arg(*p_arg, Int64_dom_t);
        break;
    case INT_SPEC_SIZE:
        if (u) val.uz = (size_t) Va_arg(*p_arg, Size_dom_t);
        else val.iz = (ptrdiff_t) Va_arg(*p_arg, Ptrdiff_dom_t);
        break;
    case INT_SPEC_MAX:
        if (u) val.uj = (uintmax_t) Va_arg(*p_arg, Uintmax_dom_t);
        else val.ij = (intmax_t) Va_arg(*p_arg, Intmax_dom_t);
        break;
    case INT_SPEC_SHRTSHRT:
        if (u) val.uhh = Va_arg(*p_arg, unsigned);
        else val.ihh = Va_arg(*p_arg, int);
        break;
    case INT_SPEC_SHRT:
        if (u) val.uh = Va_arg(*p_arg, unsigned);
        else val.ih = Va_arg(*p_arg, int);
        break;
    case INT_SPEC_LONG:
        if (u) val.ul = Va_arg(*p_arg, unsigned long);
        else val.il = Va_arg(*p_arg, long);
        break;
    case INT_SPEC_LONGLONG:
        if (u) val.ull = Va_arg(*p_arg, unsigned long long);
        else val.ill = Va_arg(*p_arg, long long);
        */
    case INT_SPEC_DEFAULT:
        if (u) val.uj = ARG_FETCH(ptr, p_arg, unsigned, unsigned);
        else val.ij = ARG_FETCH(ptr, p_arg, int, int);
        break;
    case INT_SPEC_BYTE:
        if (u) val.uj = ARG_FETCH(ptr, p_arg, uint8_t, Uint8_dom_t);
        else val.ij = ARG_FETCH(ptr, p_arg, int8_t, Int8_dom_t);
        break;
    case INT_SPEC_WORD:
        if (u) val.uj = ARG_FETCH(ptr, p_arg, uint16_t, Uint16_dom_t);
        else val.ij = ARG_FETCH(ptr, p_arg, int16_t, Int16_dom_t);
        break;
    case INT_SPEC_DWORD:
        if (u) val.uj = Va_arg(*p_arg, Uint32_dom_t);
        else val.ij = Va_arg(*p_arg, Int32_dom_t);
        break;
    case INT_SPEC_QWORD:
        if (u) val.uj = Va_arg(*p_arg, Uint64_dom_t);
        else val.ij = Va_arg(*p_arg, Int64_dom_t);
        break;
    case INT_SPEC_SIZE:
        if (u) val.uj = Va_arg(*p_arg, Size_dom_t);
        else val.ij = Va_arg(*p_arg, Ptrdiff_dom_t);
        break;
    case INT_SPEC_MAX:
        if (u) val.uj = Va_arg(*p_arg, Uintmax_dom_t);
        else val.ij = Va_arg(*p_arg, Intmax_dom_t);
        break;
    case INT_SPEC_SSHRT:
        if (u) val.uj = Va_arg(*p_arg, unsigned);
        else val.ij = Va_arg(*p_arg, int);
        break;
    case INT_SPEC_SHRT:
        if (u) val.uj = Va_arg(*p_arg, unsigned);
        else val.ij = Va_arg(*p_arg, int);
        break;
    case INT_SPEC_LONG:
        if (u) val.uj = Va_arg(*p_arg, unsigned long);
        else val.ij = Va_arg(*p_arg, long);
        break;
    case INT_SPEC_LLONG:
        if (u) val.uj = Va_arg(*p_arg, unsigned long long);
        else val.ij = Va_arg(*p_arg, long long);
    }
    if (flags & FMT_EXE_FLAG_PHONY) *p_cnt = 0;
    else
    {
        bool sgn = 1;
        if (!u)
        {
            sgn &= val.ij >= 0;
            val.uj = (uintmax_t) (sgn ? val.ij : -val.ij);
        }
        int tmp = snprintf(buff, *p_cnt, (int_flags & INT_FLAG_HEX ? int_flags & INT_FLAG_UPPERCASE ? "-%jX" : "-%jx" : "-%ju") + sgn, val.uj);
        if (tmp < 0) return 0;
        *p_cnt = (size_t) tmp;
    }
    return 1;
}

static void fmt_execute_str(size_t len, enum fmt_arg_mode mode, char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    const char *str = Va_arg(*p_arg, const char *);
    if (mode == ARG_FETCH) len = Va_arg(*p_arg, Size_dom_t);
    if (!(flags & FMT_EXE_FLAG_PHONY)) print(buff, p_cnt, str, mode == ARG_DEFAULT ? Strnlen(str, *p_cnt) : len);
}

static bool fmt_execute_time_diff(char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    uint64_t start = Va_arg(*p_arg, Uint64_dom_t), stop = Va_arg(*p_arg, Uint64_dom_t);
    if (flags & FMT_EXE_FLAG_PHONY) return 1;
    return print_time_diff(buff, p_cnt, start, stop);
}

static void fmt_execute_crt(char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    Errno_t err = Va_arg(*p_arg, Errno_t);
    if (!(flags & FMT_EXE_FLAG_PHONY)) print_crt(buff, p_cnt, err);
}

static void fmt_execute_default(size_t len, enum fmt_arg_mode mode, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    if (mode == ARG_FETCH) len = Va_arg(*p_arg, Size_dom_t);
    if (!(flags & FMT_EXE_FLAG_PHONY)) *p_cnt = len;
}

static void fmt_execute_char(size_t rep, enum fmt_arg_mode mode, char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    int ch = Va_arg(*p_arg, int);
    if (mode == ARG_FETCH) rep = Va_arg(*p_arg, Size_dom_t);
    if (flags & FMT_EXE_FLAG_PHONY) return;
    if (mode == ARG_DEFAULT) rep = 1;
    if (rep <= *p_cnt) memset(buff, ch, rep * sizeof(buff));
    *p_cnt = rep;
}

static bool fmt_execute(char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    const char *fmt = Va_arg(*p_arg, const char *);
    if (!fmt)
    {
        if (!(flags & FMT_EXE_FLAG_PHONY)) *p_cnt = 0;
        return 1;
    }
    enum fmt_execute_flags tf = 0;
    size_t cnt = 0;
    struct env env = { { 0 } };
    struct fmt_res res = { 0 };
    for (size_t i = 0, len = *p_cnt, tmp = len, pos = 0, off = 0; ++i;) switch (i)
    {
    case 4:
    case 6:
    case 8:
        cnt = size_add_sat(cnt, len);
    case 2:
        tmp = len = size_sub_sat(tmp, len);
        if (!tmp) i = SIZE_MAX; // Exit loop if there is no more space
        break;
    case 9:
        if (res.flags & FMT_LEFT_JUSTIFY)
        {
            size_t diff = cnt - off, disp = size_sub_sat(off, diff);
            memmove(buff + disp, buff + off, diff * sizeof(*buff));
            cnt = res.flags & FMT_OVERPRINT ? disp : disp + diff;
            tmp = len = *p_cnt - cnt;
        }
        else if (res.flags & FMT_OVERPRINT) tmp = len = *p_cnt - (cnt = off);
        i = 1;
    case 1:
        off = Strchrnul(fmt + pos, '%');
        if (fmt[pos + off])
        {
            print_unterminated(buff + cnt, &len, fmt + pos, off);
            pos += off + 1;
        }
        else
        {
            print(buff + cnt, &len, fmt + pos, off);
            i = SIZE_MAX; // Exit loop if there are no more arguments
        }
        off = cnt = size_add_sat(cnt, len);
        break;
    case 3:
        if (!fmt_decode(&res, fmt, &pos)) return 0;
        tf = flags;
        if ((res.flags & FMT_CONDITIONAL) && !Va_arg(*p_arg, int)) tf |= FMT_EXE_FLAG_PHONY;
        if (res.flags & (FMT_ENV_BEGIN | FMT_ENV_END))
        {
            env = Va_arg(*p_arg, struct env);
            if (!(tf & FMT_EXE_FLAG_PHONY) && (res.flags & FMT_ENV_BEGIN)) print_unterminated(buff + cnt, &len, STRL(env.begin));
            else i++;
        }
        else i++;
        break;    
    case 5:
        switch (res.type)
        {
        case TYPE_INT:
            if (!fmt_execute_int(res.int_arg.spec, res.int_arg.flags, buff + cnt, &len, p_arg, tf)) return 0;
            break;
        case TYPE_STR: 
            fmt_execute_str(res.str_arg.len, res.str_arg.mode, buff + cnt, &len, p_arg, tf);
            break;
        case TYPE_TIME_DIFF:
            if (!fmt_execute_time_diff(buff + cnt, &len, p_arg, tf)) return 0;
            break;
        case TYPE_TIME_STAMP:
            if (!(tf & FMT_EXE_FLAG_PHONY)) print_time_stamp(buff + cnt, &len);
            break;
        case TYPE_CRT:
            fmt_execute_crt(buff + cnt, &len, p_arg, tf);
            break;
        case TYPE_FMT:
            if (!fmt_execute(buff + cnt, &len, p_arg, tf)) return 0;
            break;
        case TYPE_DEFAULT:
            fmt_execute_default(res.str_arg.len, res.str_arg.mode, &len, p_arg, tf);
            break;
        case TYPE_CALLBACK:
            if (!Va_arg(*p_arg, fmt_callback)(buff + cnt, &len, p_arg, tf)) return 0;
            break;
        case TYPE_CHAR:
            fmt_execute_char(res.char_arg.rep, res.char_arg.mode, buff + cnt, &len, p_arg, tf);
            break;
        case TYPE_FLT: // NOT IMPLEMENTED!
        case TYPE_PERC:
        case TYPE_UTF:
            if (!(tf & FMT_EXE_FLAG_PHONY)) i++;
        }
        if (tf & FMT_EXE_FLAG_PHONY) i++;
        break;
    case 7:        
        if (!(tf & FMT_EXE_FLAG_PHONY) && (res.flags & FMT_ENV_END)) print_unterminated(buff + cnt, &len, STRL(env.end));
        else i++;
    }
    if (!(flags & FMT_EXE_FLAG_PHONY)) *p_cnt = cnt;
    return 1;
}

bool print_fmt(char *buff, size_t *p_cnt, ...)
{
    Va_list arg;
    Va_start(arg, p_cnt);
    bool res = fmt_execute(buff, p_cnt, &arg, 0);
    Va_end(arg);
    return res;
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
            if (bom) memcpy(log->buff, UTF8_BOM, (log->cnt = lengthof(UTF8_BOM)) * sizeof(*log->buff));
            else log->cnt = 0;
            log->style = tty ? style : (struct style) { .num = { 0 } };
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

static bool log_prefix(char *buff, size_t *p_cnt, struct code_metric metric, enum message_type type, struct style style)
{
    const struct strl ttl[] = { STRI("MESSAGE"), STRI("ERROR"), STRI("WARNING"), STRI("NOTE"), STRI("INFO"), STRI("DAMNATION") };
    return print_fmt(buff, p_cnt, "%<[%D]%> %<>s* %<(" UTF8_LDQUO "%s*" UTF8_RDQUO " @ " UTF8_LDQUO "%s*" UTF8_RDQUO ":%uz):%> ",
        style.inf, style.inf, style.ttl[type], STRL(ttl[type]), style.inf, STRL(metric.func), STRL(metric.path), metric.line, style.inf);
}

static bool log_message_impl(struct log *restrict log, struct code_metric metric, enum message_type type, union message_callback handler, void *context, Va_list *p_arg)
{
    if (!log) return 1;
    size_t cnt = log->cnt;
    for (unsigned i = 0; i < 2; i++) for (;;)
    {
        size_t len = log->cap - cnt;
        switch (i)
        {
        case 0:
            if (!log_prefix(log->buff + cnt, &len, metric, type, log->style)) return 0;
            break;
        case 1:
            if (p_arg)
            {
                Va_list arg;
                Va_copy(arg, *p_arg);
                bool res = handler.var(log->buff + cnt, &len, context, log->style, arg);
                Va_end(arg);
                if (!res) return 0;
            }
            else if (!handler.ord(log->buff + cnt, &len, context, log->style)) return 0;
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

bool log_message(struct log *restrict log, struct code_metric code_metric, enum message_type type, message_callback handler, void *context)
{
    return log_message_impl(log, code_metric, type, (union message_callback) { .ord = handler }, context, NULL);
}

bool message_var(char *buff, size_t *p_cnt, void *context, struct style style, Va_list arg)
{
    (void) style, (void) context;
    return fmt_execute(buff, p_cnt, &arg, 0);
}

bool log_message_fmt(struct log *restrict log, struct code_metric code_metric, enum message_type type, ...)
{
    Va_list arg;
    Va_start(arg, type);
    bool res = log_message_impl(log, code_metric, type, (union message_callback) { .var = message_var }, NULL, &arg);
    Va_end(arg);
    return res;
}

bool log_message_crt(struct log *restrict log, struct code_metric code_metric, enum message_type type, Errno_t err)
{
    return log_message_fmt(log, code_metric, type, "%C!\n", err);
}

bool log_message_fopen(struct log *restrict log, struct code_metric code_metric, enum message_type type, const char *restrict path, Errno_t err)
{
    return log_message_fmt(log, code_metric, type, "Unable to open the file %<>s: %C!\n", log->style.pth, path, err);
}

bool log_message_fseek(struct log *restrict log, struct code_metric code_metric, enum message_type type, int64_t offset, const char *restrict path)
{
    return log_message_fmt(log, code_metric, type, "Unable to seek into the position %<>uq of the file %<>s!\n", log->style.num, offset, log->style.pth, path);
}
