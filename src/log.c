#include "np.h"
#include "ll.h"
#include "log.h"
#include "memory.h"
#include "intdom.h"

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// In: '*p_cnt' -- size of the buffer
// Out: '*p_cnt' -- length of the string to be written (optional null-terminator is not taken in account)
void print(char *buff, size_t *p_cnt, const char *str, size_t len, bool term)
{
    size_t cnt = *p_cnt;
    *p_cnt = len;
    if (term ? !cnt || cnt - 1 < len : cnt < len) return;
    memcpy(buff, str, len * sizeof(*buff));
    if (term) buff[len] = '\0';
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

static bool fmt_execute_time_diff_sec(char *buff, size_t *p_cnt, void *p_arg, const struct env *env, enum fmt_execute_flags flags)
{
    (void) env;
    uint32_t sdr = ARG_FETCH(flags & FMT_EXE_FLAG_PTR, p_arg, uint32_t, Uint32_dom_t);
    if (flags & FMT_EXE_FLAG_PHONY) return 1;
    size_t dig = 0;
    for (uint32_t i = sdr; !(i % 10); dig++, i /= 10);
    return print_fmt(buff, p_cnt, NULL, ".000000%-ud%!-*", sdr, dig);
}

bool print_time_diff(char *buff, size_t *p_cnt, uint64_t start, uint64_t stop, const struct env *env)
{
    uint64_t diff = DIST(stop, start), hdq = diff / 3600000000;
    uint32_t hdr = (uint32_t) (diff % 3600000000), mdq = hdr / 60000000, mdr = hdr % 60000000, sdq = mdr / 1000000, sdr = mdr % 1000000;
    return env ?
        print_fmt(buff, p_cnt, NULL, "%?$%?$%s*%ud%?&%s* sec", !!hdq, "%~~uq hr ", env, hdq, !!mdq, "%~~ud min ", env, mdq, STRL(env->begin), sdq, !!sdr, fmt_execute_time_diff_sec, sdr, STRL(env->end)) :
        print_fmt(buff, p_cnt, NULL, "%?$%?$%ud%?& sec", !!hdq, "%uq hr ", hdq, !!mdq, "%ud min ", mdq, sdq, !!sdr, fmt_execute_time_diff_sec, sdr);
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
    TYPE_PATH,
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
    FMT_ENV = 4,
    FMT_ENV_USER = 8,
    FMT_LEFT_JUSTIFY = 16,
    FMT_ARG_PTR = 32,
    FMT_LIST_PTR = 64
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
    if (tmp == fmt + pos) return 1; // No argument provided
    if (!cvt) return 0;
    *p_pos = tmp - fmt + (fmt[tmp - fmt] == ';'); // Skipping optional delimiter
    *p_mode = ARG_SET;
    return 1;
}

static bool fmt_decode_utf(uint32_t *p_val, enum fmt_arg_mode *p_mode, const char *fmt, size_t *p_pos)
{
    bool hex = 0;
    size_t pos = *p_pos;
    switch (fmt[pos])
    {
    case '*':
        *p_mode = ARG_FETCH;
        *p_pos = pos + 1;
        return 1;
    case 'x':
    case 'X':
        pos++;
        hex = 1;
    }
    const char *tmp;
    unsigned cvt = (hex ? str_to_uint32_hex : str_to_uint32)(fmt + pos, &tmp, p_val);
    if (tmp == fmt + pos) return 1; // No argument provided
    if (!cvt || *p_val >= UTF8_BOUND) return 0;
    *p_pos = tmp - fmt + (fmt[tmp - fmt] == ';'); // Skipping optional delimiter
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
        case '~':
            tmp = res->flags & FMT_ENV ? res->flags & FMT_ENV_USER ? 0 : FMT_ENV_USER : FMT_ENV;
            break;
        case '-':
            tmp = FMT_LEFT_JUSTIFY;
            break;
        case '@':
            tmp = res->flags & FMT_ARG_PTR ? res->flags & FMT_LIST_PTR ? 0 : FMT_LIST_PTR : FMT_ARG_PTR;
            break;
        }
        if (!tmp) break;
        if (res->flags >= tmp) return 1;
        res->flags |= tmp;
    }
    switch (fmt[pos++])
    {
    case 'u':
        res->int_arg.flags |= INT_FLAG_UNSIGNED;
    case 'i':
        res->type = TYPE_INT;
        if (!fmt_decode_int(&res->int_arg.spec, &res->int_arg.flags, fmt, &pos)) return 0;
        break;
    case 'c':
        res->type = TYPE_CHAR;
        if (!fmt_decode_str(&res->char_arg.rep, &res->char_arg.mode, fmt, &pos)) return 0;
        break;
    case 's':
        res->type = TYPE_STR;
        if (!fmt_decode_str(&res->str_arg.len, &res->str_arg.mode, fmt, &pos)) return 0;
        break;
    case 'P':
        res->type = TYPE_PATH;
        if (!fmt_decode_str(&res->str_arg.len, &res->str_arg.mode, fmt, &pos)) return 0;
        break;
    case 'A':
        res->flt_arg.flags |= FLT_FLAG_UPPERCASE;
    case 'a':
        res->flt_arg.flags |= FLT_FLAG_HEX;
        res->flt_arg.spec = FLT_SPEC_EXP;
    case 'f':
        res->type = TYPE_FLT;
        if (!fmt_decode_str(&res->flt_arg.prec, &res->flt_arg.mode, fmt, &pos)) return 0;
        break;
    case 'E':
        res->flt_arg.flags |= FLT_FLAG_UPPERCASE;
    case 'e':
        res->flt_arg.spec = FLT_SPEC_EXP;
        res->type = TYPE_FLT;
        if (!fmt_decode_str(&res->flt_arg.prec, &res->flt_arg.mode, fmt, &pos)) return 0;
        break;
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
        if (!fmt_decode_utf(&res->utf_arg.val, &res->utf_arg.mode, fmt, &pos)) return 0;
        break;
    case '&':
        res->type = TYPE_CALLBACK;
        break;
    default:
        pos--;
        if (!fmt_decode_str(&res->str_arg.len, &res->str_arg.mode, fmt, &pos)) return 0;
    }
    *p_pos = pos;
    return 1;
}

static bool fmt_execute_int(enum fmt_int_spec int_spec, enum fmt_int_flags int_flags, char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    bool u = int_flags & INT_FLAG_UNSIGNED, ptr = flags & FMT_EXE_FLAG_PTR;
	union int_val val = { .uj = 0 };
    switch (int_spec)
    {
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
        if (u) val.uj = ARG_FETCH(ptr, p_arg, uint32_t, Uint32_dom_t);
        else val.ij = ARG_FETCH(ptr, p_arg, int32_t, Int32_dom_t);
        break;
    case INT_SPEC_QWORD:
        if (u) val.uj = ARG_FETCH(ptr, p_arg, uint64_t, Uint64_dom_t);
        else val.ij = ARG_FETCH(ptr, p_arg, int64_t, Int64_dom_t);
        break;
    case INT_SPEC_SIZE:
        if (u) val.uj = ARG_FETCH(ptr, p_arg, size_t, Size_dom_t);
        else val.ij = ARG_FETCH(ptr, p_arg, ptrdiff_t, Ptrdiff_dom_t);
        break;
    case INT_SPEC_MAX:
        if (u) val.uj = ARG_FETCH(ptr, p_arg, uintmax_t, Uintmax_dom_t);
        else val.ij = ARG_FETCH(ptr, p_arg, intmax_t, Intmax_dom_t);
        break;
    case INT_SPEC_SSHRT:
        if (u) val.uj = ARG_FETCH(ptr, p_arg, unsigned, unsigned);
        else val.ij = ARG_FETCH(ptr, p_arg, int, int);
        break;
    case INT_SPEC_SHRT:
        if (u) val.uj = ARG_FETCH(ptr, p_arg, unsigned, unsigned);
        else val.ij = ARG_FETCH(ptr, p_arg, int, int);
        break;
    case INT_SPEC_LONG:
        if (u) val.uj = ARG_FETCH(ptr, p_arg, unsigned long, unsigned long);
        else val.ij = ARG_FETCH(ptr, p_arg, long, long);
        break;
    case INT_SPEC_LLONG:
        if (u) val.uj = ARG_FETCH(ptr, p_arg, unsigned long long, unsigned long long);
        else val.ij = ARG_FETCH(ptr, p_arg, long long, long long);
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
    bool ptr = flags & FMT_EXE_FLAG_PTR;
    const char *str = ARG_FETCH_PTR(ptr, p_arg);
    if (mode == ARG_FETCH) len = ARG_FETCH(ptr, p_arg, size_t, Size_dom_t);
    if (!(flags & FMT_EXE_FLAG_PHONY)) print(buff, p_cnt, str, mode == ARG_DEFAULT ? Strnlen(str, *p_cnt) : len, 1);
}

static bool fmt_execute_time_diff(char *buff, size_t *p_cnt, Va_list *p_arg, const struct env *env, enum fmt_execute_flags flags)
{
    bool ptr = flags & FMT_EXE_FLAG_PTR;
    uint64_t start = ARG_FETCH(ptr, p_arg, uint64_t, Uint64_dom_t), stop = ARG_FETCH(ptr, p_arg, uint64_t, Uint64_dom_t);
    if (flags & FMT_EXE_FLAG_PHONY) return 1;
    return print_time_diff(buff, p_cnt, start, stop, env);
}

static void fmt_execute_crt(char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    Errno_t err = ARG_FETCH(flags & FMT_EXE_FLAG_PTR, p_arg, Errno_t, Errno_dom_t);
    if (!(flags & FMT_EXE_FLAG_PHONY)) print_crt(buff, p_cnt, err);
}

static void fmt_execute_default(size_t len, enum fmt_arg_mode mode, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    if (mode == ARG_FETCH) len = ARG_FETCH(flags & FMT_EXE_FLAG_PTR, p_arg, size_t, Size_dom_t);
    if (!(flags & FMT_EXE_FLAG_PHONY)) *p_cnt = len;
}

static void fmt_execute_char(size_t rep, enum fmt_arg_mode mode, char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    bool ptr = flags & FMT_EXE_FLAG_PTR;
    int ch = ARG_FETCH(ptr, p_arg, char, int);
    if (mode == ARG_FETCH) rep = ARG_FETCH(ptr, p_arg, size_t, Size_dom_t);
    if (flags & FMT_EXE_FLAG_PHONY) return;
    if (mode == ARG_DEFAULT) rep = 1;
    if (rep <= *p_cnt) memset(buff, ch, rep * sizeof(buff));
    *p_cnt = rep;
}

static void fmt_execute_utf(uint32_t val, enum fmt_arg_mode mode, char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    if (mode == ARG_DEFAULT) return;
    if (mode == ARG_FETCH) val = ARG_FETCH(flags & FMT_EXE_FLAG_PTR, p_arg, uint32_t, Uint32_dom_t);
    if (flags & FMT_EXE_FLAG_PHONY) return;
    uint8_t str[UTF8_COUNT], len;
    utf8_encode(val, str, &len);
    print(buff, p_cnt, (char *) str, len, 0);
}

static bool fmt_execute(char *buff, size_t *p_cnt, void *p_arg, const struct style *style, enum fmt_execute_flags flags)
{
    const void *p_arg_list = NULL;
    void *p_arg_sub = NULL;
    bool ptr = flags & FMT_EXE_FLAG_PTR, list_ptr = 0;
    const char *fmt = ARG_FETCH_PTR(ptr, p_arg);
    if (!fmt)
    {
        if (!(flags & FMT_EXE_FLAG_PHONY)) *p_cnt = 0;
        return 1;
    }
    Va_list arg_sub;
    enum fmt_execute_flags tf = 0;
    size_t cnt = 0;
    const struct env *env;
    struct fmt_res res = { 0 };
    for (size_t i = 0, len = *p_cnt, tmp = len, pos = 0, off = 0; ++i;) switch (i)
    {
    case 1:
        off = Strchrnul(fmt + pos, '%');
        if (fmt[pos + off])
        {
            print(buff + cnt, &len, fmt + pos, off, 0);
            pos += off + 1;
            off = cnt = size_add_sat(cnt, len);
            tmp = len = size_sub_sat(tmp, len);
            if (!tmp) i = SIZE_MAX; // Exit the loop if there is no more space
            break;
        }
        print(buff + cnt, &len, fmt + pos, off, 1);
        cnt = size_add_sat(cnt, len);
        i = SIZE_MAX; // Exit loop if there are no more arguments
        break;
    case 2:
        if (!fmt_decode(&res, fmt, &pos)) return 0;
        tf = flags;
        list_ptr = res.flags & FMT_LIST_PTR;
        if (list_ptr)
        {
            p_arg_list = ARG_FETCH_PTR(ptr, p_arg);
            p_arg_sub = &p_arg_list; // Here MSVC promts warning that should be dismissed
            tf |= FMT_EXE_FLAG_PTR;
            list_ptr = list_ptr || ptr;
        }
        else if (res.flags & FMT_ARG_PTR)
        {
            Va_copy(arg_sub, *(Va_list *) ARG_FETCH_PTR(ptr, p_arg));
            p_arg_sub = &arg_sub;
        }
        else
        {
            p_arg_sub = p_arg;
            list_ptr = ptr;
        }
        if ((res.flags & FMT_CONDITIONAL) && !ARG_FETCH(list_ptr, p_arg_sub, bool, int)) tf |= FMT_EXE_FLAG_PHONY;
        if (res.flags & FMT_ENV_USER) env = ARG_FETCH_PTR(list_ptr, p_arg_sub);
        else if (style && (res.flags & FMT_ENV)) switch (res.type)
        {
        case TYPE_INT:
            env = &style->type_int;
            break;
        case TYPE_CHAR:
            env = &style->type_char;
            break;
        case TYPE_PATH:
            env = &style->type_path;
            break;
        case TYPE_STR:
            env = &style->type_str;
            break;
        case TYPE_FLT:
            env = &style->type_flt;
            break;
        case TYPE_TIME_DIFF:
            env = &style->type_time_diff;
            break;
        case TYPE_TIME_STAMP:
            env = &style->type_time_stamp;
            break;
        case TYPE_UTF:
            env = &style->type_utf;
            break;
        default:
            env = NULL;
        }
        else env = NULL;
        switch (res.type)
        {
        case TYPE_FMT:
        case TYPE_TIME_DIFF:
        case TYPE_CALLBACK:
            for (size_t j = 0; ++j;) switch (j)
            {
            case 1:
                switch (res.type)
                {
                case TYPE_FMT:
                    if (!fmt_execute(buff + cnt, &len, p_arg_sub, style, tf)) i = SIZE_MAX; // Exit on error
                    break;
                case TYPE_TIME_DIFF:
                    if (!fmt_execute_time_diff(buff + cnt, &len, p_arg_sub, env, tf)) i = SIZE_MAX; // Exit on error
                    break;
                case TYPE_CALLBACK:
                    if (!ARG_FETCH(list_ptr, p_arg_sub, fmt_callback, fmt_callback)(buff + cnt, &len, p_arg_sub, env, tf)) i = SIZE_MAX; // Exit on error
                    break;
                default:
                    if (!(tf & FMT_EXE_FLAG_PHONY)) j++;
                }
                if (!(i + 1)) j = SIZE_MAX;
                else if (tf & FMT_EXE_FLAG_PHONY) j++;
                break;
            case 2:
                cnt = size_add_sat(cnt, len);
                tmp = len = size_sub_sat(tmp, len);
            case 3:
                j = SIZE_MAX;
            }
            break;
        default:
            for (size_t j = 0; ++j;) switch (j)
            {
            case 1:
                if (env && !(tf & FMT_EXE_FLAG_PHONY)) print(buff + cnt, &len, STRL(env->begin), 0);
                else j++;
                break;
            case 2:
            case 4:            
                cnt = size_add_sat(cnt, len);
                tmp = len = size_sub_sat(tmp, len);
                if (!tmp) j = SIZE_MAX;
                break;            
            case 3:
                switch (res.type)
                {
                case TYPE_INT:
                    if (!fmt_execute_int(res.int_arg.spec, res.int_arg.flags, buff + cnt, &len, p_arg_sub, tf)) i = SIZE_MAX; // Exit on error
                    break;
                case TYPE_STR:
                case TYPE_PATH:
                    fmt_execute_str(res.str_arg.len, res.str_arg.mode, buff + cnt, &len, p_arg_sub, tf);
                    break;
                case TYPE_TIME_STAMP:
                    if (!(tf & FMT_EXE_FLAG_PHONY)) print_time_stamp(buff + cnt, &len);
                    break;
                case TYPE_CRT:
                    fmt_execute_crt(buff + cnt, &len, p_arg_sub, tf);
                    break;
                case TYPE_DEFAULT:
                    fmt_execute_default(res.str_arg.len, res.str_arg.mode, &len, p_arg_sub, tf);
                    break;
                case TYPE_CHAR:
                    fmt_execute_char(res.char_arg.rep, res.char_arg.mode, buff + cnt, &len, p_arg_sub, tf);
                    break;
                case TYPE_PERC:
                    if (!(tf & FMT_EXE_FLAG_PHONY)) print(buff + cnt, &len, "%", 1, 0);
                    break;
                case TYPE_UTF:
                    fmt_execute_utf(res.utf_arg.val, res.utf_arg.mode, buff + cnt, &len, p_arg_sub, tf);
                    break;
                case TYPE_FLT: // NOT IMPLEMENTED!
                default:
                    if (!(tf & FMT_EXE_FLAG_PHONY)) j++;
                }
                if (!(i + 1)) j = SIZE_MAX;
                else if (tf & FMT_EXE_FLAG_PHONY) j++;
                break;
            case 5:
                if (env && !(tf & FMT_EXE_FLAG_PHONY)) print(buff + cnt, &len, STRL(env->end), 0);
                else j++;
                break;
            case 6:
                cnt = size_add_sat(cnt, len);
                tmp = len = size_sub_sat(tmp, len);
            case 7:
                j = SIZE_MAX;
            } 
        }
        if (res.flags & FMT_ARG_PTR) Va_end(arg_sub);
        if (!(i + 1)) return 0;
        if (!tmp) i = SIZE_MAX;
        break;
    case 3:
        if (res.flags & FMT_LEFT_JUSTIFY)
        {
            size_t diff = cnt - off, disp = size_sub_sat(off, diff);
            memmove(buff + disp, buff + off, diff * sizeof(*buff));
            cnt = res.flags & FMT_OVERPRINT ? disp : disp + diff;
            tmp = len = *p_cnt - cnt;
        }
        else if (res.flags & FMT_OVERPRINT) tmp = len = *p_cnt - (cnt = off);
        i = 0; // Return to beginning
    }
    if (!(flags & FMT_EXE_FLAG_PHONY)) *p_cnt = cnt;
    return 1;
}

bool print_fmt(char *buff, size_t *p_cnt, const struct style *style, ...)
{
    Va_list arg;
    Va_start(arg, style);
    bool res = fmt_execute(buff, p_cnt, &arg, style, 0);
    Va_end(arg);
    return res;
}

// Last argument may be 'NULL'
bool log_init(struct log *restrict log, char *restrict path, size_t lim, enum log_flags flags, const struct ttl_style *restrict ttl_style, const struct style *restrict style, struct log *restrict log_error)
{
    static const struct ttl_style def_ttl_style = { { { 0 } } };
    static const struct style def_style = { .type_str = ENV_INIT(UTF8_LDQUO, UTF8_RDQUO), .type_char = ENV_INIT(UTF8_LSQUO, UTF8_RSQUO) };

    FILE *f = path ? fopen(path, flags & LOG_APPEND ? "a" : "w") : stderr;
    if (path && !f) log_message_fopen(log_error, CODE_METRIC, MESSAGE_ERROR, path, errno);
    else
    {
        bool tty = (flags & (LOG_FORCE_TTY)) || file_is_tty(f), bom = !(tty || (flags & (LOG_NO_BOM | LOG_APPEND)));
        size_t cap = bom ? MAX(lim, lengthof(UTF8_BOM)) : lim;
        if (array_assert(log_error, CODE_METRIC, array_init(&log->buff, &log->cap, cap, sizeof(*log->buff), 0, 0)))
        {
            if (bom) memcpy(log->buff, UTF8_BOM, (log->cnt = lengthof(UTF8_BOM)) * sizeof(*log->buff));
            else log->cnt = 0;
            log->ttl_style = tty ? ttl_style : &def_ttl_style;
            log->style = tty ? style : &def_style;
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
    if (!cnt) return 1;
    size_t wr = fwrite(log->buff, 1, cnt, log->file);
    log->tot += wr;
    log->cnt = 0;
    return !fflush(log->file) && wr == cnt;
}

// May be used for 'log' allocated by 'calloc' (or filled with zeros statically)
void log_close(struct log *restrict log)
{
    log_flush(log);
    Fclose(log->file);
    free(log->buff);
}

bool log_multiple_init(struct log *restrict arr, size_t cnt, char *restrict path, size_t cap, enum log_flags flags, const struct ttl_style *restrict ttl_style, const struct style *restrict style, struct log *restrict log_error)
{
    size_t i = 0;
    for (; i < cnt && log_init(arr + i, path, cap, flags | LOG_APPEND, ttl_style, style, log_error); i++);
    if (i == cnt) return 1;
    for (; --i; log_close(arr + i));
    return 0;
}

void log_multiple_close(struct log *restrict arr, size_t cnt)
{
    if (cnt) for (size_t i = cnt; --i; log_close(arr + i));
}

static bool log_prefix(char *buff, size_t *p_cnt, struct code_metric metric, enum message_type type, const struct ttl_style *ttl_style)
{
    const struct strl ttl[] = { STRI("MESSAGE"), STRI("ERROR"), STRI("WARNING"), STRI("NOTE"), STRI("INFO"), STRI("DAMNATION") };
    return print_fmt(buff, p_cnt, NULL, "%s*[%D]%s* %~~s* %s*(" UTF8_LSQUO "%s*" UTF8_RSQUO " @ " UTF8_LDQUO "%s*" UTF8_RDQUO ":%uz):%s* ",
        STRL(ttl_style->time_stamp.begin),
        STRL(ttl_style->time_stamp.end),
        ttl_style->header + type, STRL(ttl[type]),
        STRL(ttl_style->code_metric.begin),
        STRL(metric.func), 
        STRL(metric.path), 
        metric.line, 
        STRL(ttl_style->code_metric.end));
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
            if (!log_prefix(log->buff + cnt, &len, metric, type, log->ttl_style)) return 0;
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
        struct array_result res = array_test(&log->buff, &log->cap, sizeof(*log->buff), 0, 0, cnt, len, 1); // All messages are NULL-terminated
        if (!res.status) return 0;
        if (!(res.status & ARRAY_UNTOUCHED)) continue;
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

bool message_var(char *buff, size_t *p_cnt, void *context, const struct style *style, Va_list arg)
{
    (void) style, (void) context;
    return fmt_execute(buff, p_cnt, &arg, style, 0);
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
    return log_message_fmt(log, code_metric, type, "Unable to open the file %~P. %C!\n", path, err);
}

bool log_message_fseek(struct log *restrict log, struct code_metric code_metric, enum message_type type, int64_t offset, const char *restrict path)
{
    return log_message_fmt(log, code_metric, type, "Unable to seek into the position %~uq of the file %~P!\n", offset, path);
}

bool array_assert(struct log *log, struct code_metric code_metric, struct array_result res)
{
    switch (res.error)
    {
    case ARRAY_OUT_OF_MEMORY:
        log_message_fmt(log, code_metric, MESSAGE_ERROR, "Unable to allocate %~uz bytes of memory! %C!\n", res.tot, errno);
        break;
    case ARRAY_OVERFLOW:
        log_message_fmt(log, code_metric, MESSAGE_ERROR, "Memory allocation failed! %C!\n", ERANGE);
        break;
    case ARRAY_NO_ERROR:
        break;
    }
    return res.status;
}

bool crt_assert(struct log *log, struct code_metric code_metric, bool res)
{
    if (!res) log_message_fmt(log, code_metric, MESSAGE_ERROR, "%C!\n", errno);
    return res;
}

#ifdef _WIN32

#   include <windows.h>

static bool fmt_execute_wstr(char *buff, size_t *p_cnt, void *p_arg, const struct env *env, enum fmt_execute_flags flags)
{
    (void) env;
    bool ptr = flags & FMT_EXE_FLAG_PTR;
    const wchar_t *wstr = ARG_FETCH_PTR(ptr, p_arg);
    size_t wlen = ARG_FETCH(ptr, p_arg, size_t, Size_dom_t);
    if (flags & FMT_EXE_FLAG_PHONY) return 1;
    uint32_t val;
    uint8_t context = 0;
    uint8_t str[UTF8_COUNT];
    size_t len = 0, cnt = *p_cnt;
    for (size_t i = 0; i < wlen; i++)
    {
        if (!utf16_decode((uint16_t) wstr[i], &val, NULL, NULL, &context)) return 0;
        if (context) continue;
        uint8_t ulen;
        utf8_encode(val, str, &ulen);
        size_t tmp = cnt;
        print(buff + len, &tmp, (char *) str, ulen, 0);
        len = size_add_sat(len, tmp);
        cnt = size_sub_sat(cnt, tmp);
        if (!cnt) break;
    }
    if (context) return 0;
    *p_cnt = len;
    return 1;
}

static bool fmt_execute_wapi(char *buff, size_t *p_cnt, void *p_arg, const struct env *env, enum fmt_execute_flags flags)
{
    (void) env;
    uint32_t err = ARG_FETCH(flags & FMT_EXE_FLAG_PTR, p_arg, uint32_t, Uint32_dom_t);
    if (flags & FMT_EXE_FLAG_PHONY) return 1;
    wchar_t *wstr = NULL;
    size_t wlen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, (DWORD) err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR) &wstr, 0, NULL);
    if (!wstr) return 0;
    bool res = print_fmt(buff, p_cnt, NULL, "%&", fmt_execute_wstr, wstr, size_sub_sat(wlen, lengthof(".\r\n"))); // Cutting ".\r\n" at the end of the string
    LocalFree(wstr);
    return res;
}

bool wapi_assert(struct log *log, struct code_metric code_metric, bool res)
{
    if (!res) log_message_fmt(log, code_metric, MESSAGE_ERROR, "%&!\n", fmt_execute_wapi, (uint32_t) GetLastError());
    return res;
}

#else

bool wapi_assert(struct log *log, struct code_metric code_metric, bool res)
{
    log_message_fmt(log, code_metric, MESSAGE_RESERVED, "Function %~~s* should be called only when %s*_WIN32%s* macro is defined!\n",
        &log->style->type_char,
        STRC(__func__),
        STRL(log->style->type_char.begin),
        STRL(log->style->type_char.end));
    return res;
}

#endif
