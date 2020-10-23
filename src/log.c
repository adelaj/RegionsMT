#define VERBOSE
#include "np.h"
#include "ll.h"
#include "log.h"
#include "memory.h"
#include "intdom.h"
#include "threadsupp.h"
#undef VERBOSE

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

DECL_PTR_ARG_FETCH(ptr, void)
DECL_PTR_ARG_FETCH(env_ptr, struct env)
DECL_PTR_ARG_FETCH(style_ptr, struct style)
DECL_PTR_ARG_FETCH(Va_list_ptr, Va_list)

DECL_ARG_FETCH(ussint, unsigned char, unsigned)
DECL_ARG_FETCH(usint, unsigned short, unsigned)
DECL_ARG_FETCH(uint, unsigned, unsigned)
DECL_ARG_FETCH(ulint, unsigned long, unsigned long)
DECL_ARG_FETCH(ullint, unsigned long long, unsigned long long)
DECL_ARG_FETCH(uint8, uint8_t, Uint8_dom_t)
DECL_ARG_FETCH(uint16, uint16_t, Uint16_dom_t)
DECL_ARG_FETCH(uint32, uint32_t, Uint32_dom_t)
DECL_ARG_FETCH(uint64, uint64_t, Uint64_dom_t)
DECL_ARG_FETCH(size, size_t, Size_dom_t)
DECL_ARG_FETCH(uintmax, uintmax_t, Uintmax_dom_t)

DECL_ARG_FETCH(bool, bool, int)
DECL_ARG_FETCH(Errno, Errno_t, Errno_dom_t)
DECL_ARG_FETCH(ssint, signed char, int)
DECL_ARG_FETCH(sint, short, int)
DECL_ARG_FETCH(int, int, int)
DECL_ARG_FETCH(lint, long, long)
DECL_ARG_FETCH(llint, long long, long long)
DECL_ARG_FETCH(int8, int8_t, Int8_dom_t)
DECL_ARG_FETCH(int16, int16_t, Int16_dom_t)
DECL_ARG_FETCH(int32, int32_t, Int32_dom_t)
DECL_ARG_FETCH(int64, int64_t, Int64_dom_t)
DECL_ARG_FETCH(ptrdiff, ptrdiff_t, Ptrdiff_dom_t)
DECL_ARG_FETCH(intmax, intmax_t, Intmax_dom_t)

// In: '*p_cnt' -- size of the buffer
// Out: '*p_cnt' -- length of the string to be written (optional null-terminator is not taken in account)
void print(char *restrict buff, size_t *restrict p_cnt, const char *restrict str, size_t len, bool term)
{
    size_t cnt = *p_cnt;
    *p_cnt = len;
    if (term ? !cnt || cnt - 1 < len : cnt < len) return;
    memcpy(buff, str, len * sizeof(*buff));
    if (term) buff[len] = '\0';
}

void print_time_stamp(char *restrict buff, size_t *restrict p_cnt)
{
    size_t cnt = *p_cnt;
    if (cnt)
    {
        time_t t;
        time(&t);
        struct tm ts;
        Localtime_s(&ts, &t);
        size_t len = strftime(buff, cnt, "%Y-%m-%d %H:%M:%S UTC%z", &ts);
        *p_cnt = len ? len : add_sat(cnt, cnt);
    }
    else *p_cnt = 1;
}

static struct message_result fmt_execute_time_diff_sec(char *buff, size_t *p_cnt, void *p_arg, const struct env *env, enum fmt_execute_flags flags)
{
    (void) env;
    uint32_t sdr = uint32_arg_fetch(p_arg, flags & FMT_EXE_FLAG_PTR);
    if (flags & FMT_EXE_FLAG_PHONY) return MESSAGE_SUCCESS;
    size_t dig = 0;
    for (uint32_t i = sdr; !(i % 10); dig++, i /= 10);
    return print_fmt(buff, p_cnt, NULL, ".000000%-ud%!-*", sdr, dig);
}

struct message_result print_time_diff(char *buff, size_t *p_cnt, uint64_t start, uint64_t stop, const struct env *env)
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
    *p_cnt = cnt ? Strerror_s(buff, cnt, err) ? add_sat(cnt, cnt) : Strnlen(buff, cnt) : 1;
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
    TYPE_CODE_METRIC,
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

/*
enum fmt_flags2 {
    FMT_ARG_PTR = 1, // '@' -- fetch args from 'Va_list *' passed through the next argument 
    FMT_LIST_PTR = 2, // '@@' -- fetch args from 'void *[]' passed through the next argument
    FMT_RETAIN_ARG = 4, // ^ 
    FMT_CONDITIONAL = 8, // ?
    FMT_CONDITIONAL_NOT = 4, // ??
    FMT_OVERPRINT = 8,
    FMT_QUOTE_SELECT = 16,
    FMT_QUOTE_SINGLE = 32,
    FMT_QUOTE_DOUBLE = 64,
    FMT_ENV = 128,
    FMT_ENV_USER = 256,
    FMT_ARG_PTR = 512,
    FMT_LEFT_JUSTIFY = 2048,
};
*/

enum fmt_flags {
    FMT_COPY_ARGS = 1,
    FMT_CONDITIONAL = 2,
    FMT_CONDITIONAL_NOT = 4,
    FMT_OVERPRINT =  8,
    FMT_QUOTE_SELECT = 16,
    FMT_QUOTE_SINGLE = 32,
    FMT_QUOTE_DOUBLE = 64,
    FMT_ENV = 128,
    FMT_ENV_USER = 256,
    FMT_ARG_PTR = 512,
    FMT_LIST_PTR = 1024,
    FMT_LEFT_JUSTIFY = 2048,
};

enum fmt_arg_mode {
    ARG_DEFAULT = 0,
    ARG_SET,
    ARG_FETCH,
};

struct fmt_result {
    enum fmt_flags flags;
    enum fmt_type type;
    struct {
        enum fmt_arg_mode mode;
        size_t len;
    } box_arg;
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

static struct message_result fmt_decode_int(enum fmt_int_spec *p_spec, enum fmt_int_flags *p_flags, const char *fmt, size_t *p_pos)
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
            return MESSAGE_SUCCESS;
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
    return MESSAGE_SUCCESS;
}

static struct message_result fmt_decode_str(size_t *p_cnt, enum fmt_arg_mode *p_mode, const char *fmt, size_t *p_pos)
{
    size_t pos = *p_pos;
    if (fmt[pos] == '*')
    {
        *p_mode = ARG_FETCH;
        *p_pos = pos + 1;
        return MESSAGE_SUCCESS;
    }
    const char *tmp;
    unsigned cvt = str_to_size(fmt + pos, &tmp, p_cnt);
    if (tmp == fmt + pos) return MESSAGE_SUCCESS; // No argument provided
    if (!cvt) return MESSAGE_FAILURE;
    *p_pos = tmp - fmt + (fmt[tmp - fmt] == ';'); // Skipping optional delimiter
    *p_mode = ARG_SET;
    return MESSAGE_SUCCESS;
}

static struct message_result fmt_decode_utf(uint32_t *p_val, enum fmt_arg_mode *p_mode, const char *fmt, size_t *p_pos)
{
    bool hex = 0;
    size_t pos = *p_pos;
    switch (fmt[pos])
    {
    case '*':
        *p_mode = ARG_FETCH;
        *p_pos = pos + 1;
        return MESSAGE_SUCCESS;
    case 'x':
    case 'X':
        pos++;
        hex = 1;
    }
    const char *tmp;
    unsigned cvt = (hex ? str_to_uint32_hex : str_to_uint32)(fmt + pos, &tmp, p_val);
    if (tmp == fmt + pos) return MESSAGE_SUCCESS; // No argument provided
    if (!cvt || *p_val >= UTF8_BOUND) return MESSAGE_FAILURE;
    *p_pos = tmp - fmt + (fmt[tmp - fmt] == ';'); // Skipping optional delimiter
    *p_mode = ARG_SET;
    return MESSAGE_SUCCESS;
}

#define TRY_FLAG(CMP, SET, DEFAULT) ((CMP) & (SET) ? (DEFAULT) : (SET))
#define TRY_FLAG2(CMP, SET1, SET2, DEFAULT) TRY_FLAG((CMP), (SET1), TRY_FLAG((CMP), (SET2), (DEFAULT)))

static struct message_result fmt_decode(struct fmt_result *res, const char *fmt, size_t *p_pos)
{
    size_t pos = *p_pos;
    *res = (struct fmt_result) { 0 };
    struct message_result message_res = MESSAGE_SUCCESS;
    for (;; pos++)
    {
        enum fmt_flags tmp = 0;
        switch (fmt[pos])
        {
        // Warning! Cases should be sorted according to the flag values.
        case '^':
            tmp = TRY_FLAG(res->flags, FMT_COPY_ARGS, 0);
            break;
        case '?':
            tmp = TRY_FLAG2(res->flags, FMT_CONDITIONAL, FMT_CONDITIONAL_NOT, 0);
            break;
        case '!':
            tmp = TRY_FLAG(res->flags, FMT_OVERPRINT, 0);
            break;        
        case '\'':
            tmp = TRY_FLAG2(res->flags, FMT_QUOTE_SELECT, FMT_QUOTE_SINGLE, 0);
            break;
        case '\"': // Warning! "\'\'" and "\"" sequences are incompatible 
            tmp = res->flags & FMT_QUOTE_SINGLE ? 0 : FMT_QUOTE_DOUBLE;
            break;
        case '~':
            tmp = TRY_FLAG2(res->flags, FMT_ENV, FMT_ENV_USER, 0);
            break;
        case '@':
            tmp = TRY_FLAG2(res->flags, FMT_ARG_PTR, FMT_LIST_PTR, 0);
            break;
        case '-':
            tmp = TRY_FLAG(res->flags, FMT_LEFT_JUSTIFY, 0);
            break;
        }
        if (!tmp) break;
        if (res->flags >= tmp) return MESSAGE_SUCCESS;
        res->flags |= tmp;
    }
    switch (fmt[pos++])
    {
    case 'u':
        res->int_arg.flags |= INT_FLAG_UNSIGNED;
    case 'i':
        res->type = TYPE_INT;
        message_res = fmt_decode_int(&res->int_arg.spec, &res->int_arg.flags, fmt, &pos);
        break;
    case 'c':
        res->type = TYPE_CHAR;
        message_res = fmt_decode_str(&res->char_arg.rep, &res->char_arg.mode, fmt, &pos);
        break;
    case 's':
        res->type = TYPE_STR;
        message_res = fmt_decode_str(&res->str_arg.len, &res->str_arg.mode, fmt, &pos);
        break;
    case 'P':
        res->type = TYPE_PATH;
        message_res = fmt_decode_str(&res->str_arg.len, &res->str_arg.mode, fmt, &pos);
        break;
    case 'A':
        res->flt_arg.flags |= FLT_FLAG_UPPERCASE;
    case 'a':
        res->flt_arg.flags |= FLT_FLAG_HEX;
        res->flt_arg.spec = FLT_SPEC_EXP;
    case 'f':
        res->type = TYPE_FLT;
        message_res = fmt_decode_str(&res->flt_arg.prec, &res->flt_arg.mode, fmt, &pos);
        break;
    case 'E':
        res->flt_arg.flags |= FLT_FLAG_UPPERCASE;
    case 'e':
        res->flt_arg.spec = FLT_SPEC_EXP;
        res->type = TYPE_FLT;
        message_res = fmt_decode_str(&res->flt_arg.prec, &res->flt_arg.mode, fmt, &pos);
        break;
    case 'T':
        res->type = TYPE_TIME_DIFF;
        break;
    case 'M':
        res->type = TYPE_CODE_METRIC;
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
        message_res = fmt_decode_utf(&res->utf_arg.val, &res->utf_arg.mode, fmt, &pos);
        break;
    case '&':
        res->type = TYPE_CALLBACK;
        break;
    default:
        pos--;
        message_res = fmt_decode_str(&res->str_arg.len, &res->str_arg.mode, fmt, &pos);
    }
    if (!message_res.status) return message_res;
    *p_pos = pos;
    return MESSAGE_SUCCESS;
}

static struct message_result fmt_execute_int(enum fmt_int_spec int_spec, enum fmt_int_flags int_flags, char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    bool u = int_flags & INT_FLAG_UNSIGNED, ptr = flags & FMT_EXE_FLAG_PTR;
    union int_val val = { .uj = 0 };
    switch (int_spec)
    {
    case INT_SPEC_DEFAULT:
        if (u) val.u = uint_arg_fetch(p_arg, ptr);
        else val.i = int_arg_fetch(p_arg, ptr);
        break;
    case INT_SPEC_BYTE:
        if (u) val.ub = uint8_arg_fetch(p_arg, ptr);
        else val.ib = int8_arg_fetch(p_arg, ptr);
        break;
    case INT_SPEC_WORD:
        if (u) val.uw = uint16_arg_fetch(p_arg, ptr);
        else val.iw = int16_arg_fetch(p_arg, ptr);
        break;
    case INT_SPEC_DWORD:
        if (u) val.ud = uint32_arg_fetch(p_arg, ptr);
        else val.id = int32_arg_fetch(p_arg, ptr);
        break;
    case INT_SPEC_QWORD:
        if (u) val.uq = uint64_arg_fetch(p_arg, ptr);
        else val.iq = int64_arg_fetch(p_arg, ptr);
        break;
    case INT_SPEC_SIZE:
        if (u) val.uz = size_arg_fetch(p_arg, ptr);
        else val.iz = ptrdiff_arg_fetch(p_arg, ptr);
        break;
    case INT_SPEC_MAX:
        if (u) val.uj = uintmax_arg_fetch(p_arg, ptr);
        else val.ij = intmax_arg_fetch(p_arg, ptr);
        break;
    case INT_SPEC_SSHRT:
        if (u) val.uhh = ussint_arg_fetch(p_arg, ptr);
        else val.ihh = ssint_arg_fetch(p_arg, ptr);
        break;
    case INT_SPEC_SHRT:
        if (u) val.uh = usint_arg_fetch(p_arg, ptr);
        else val.ih = sint_arg_fetch(p_arg, ptr);
        break;
    case INT_SPEC_LONG:
        if (u) val.ul = ulint_arg_fetch(p_arg, ptr);
        else val.il = lint_arg_fetch(p_arg, ptr);
        break;
    case INT_SPEC_LLONG:
        if (u) val.ull = ullint_arg_fetch(p_arg, ptr);
        else val.ill = llint_arg_fetch(p_arg, ptr);
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
        if (tmp < 0) return MESSAGE_FAILURE;
        *p_cnt = (size_t) tmp;
    }
    return MESSAGE_SUCCESS;
}

static void fmt_execute_str(size_t len, enum fmt_arg_mode mode, char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    bool ptr = flags & FMT_EXE_FLAG_PTR;
    const char *str = ptr_arg_fetch(p_arg, ptr);
    if (mode == ARG_FETCH) len = size_arg_fetch(p_arg, ptr);
    if (!(flags & FMT_EXE_FLAG_PHONY)) print(buff, p_cnt, str, mode == ARG_DEFAULT ? Strnlen(str, *p_cnt) : len, 1);
}

static struct message_result fmt_execute_time_diff(char *buff, size_t *p_cnt, Va_list *p_arg, const struct env *env, enum fmt_execute_flags flags)
{
    bool ptr = flags & FMT_EXE_FLAG_PTR;
    uint64_t start = uint64_arg_fetch(p_arg, ptr), stop = uint64_arg_fetch(p_arg, ptr);
    if (flags & FMT_EXE_FLAG_PHONY) return MESSAGE_SUCCESS;
    return print_time_diff(buff, p_cnt, start, stop, env);
}

static DECL_STRUCT_ARG_FETCH(code_metric, struct code_metric)

static struct message_result fmt_execute_code_metric(char *buff, size_t *p_cnt, Va_list *p_arg, const struct env *env, enum fmt_execute_flags flags)
{
    struct code_metric metric = code_metric_arg_fetch(p_arg, flags & FMT_EXE_FLAG_PTR);
    if (flags & FMT_EXE_FLAG_PHONY) return MESSAGE_SUCCESS;
    return print_fmt(buff, p_cnt, NULL, "%''~~s* @ %\"~~s*:%~~uz", env, STRL(metric.func), env, STRL(metric.path), env, metric.line);
}

static void fmt_execute_crt(char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    Errno_t err = Errno_arg_fetch(p_arg, flags & FMT_EXE_FLAG_PTR);
    if (!(flags & FMT_EXE_FLAG_PHONY)) print_crt(buff, p_cnt, err);
}

static void fmt_execute_default(size_t len, enum fmt_arg_mode mode, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    if (mode == ARG_FETCH) len = size_arg_fetch(p_arg, flags & FMT_EXE_FLAG_PTR);
    if (!(flags & FMT_EXE_FLAG_PHONY)) *p_cnt = len;
}

static void fmt_execute_char(size_t rep, enum fmt_arg_mode mode, char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    bool ptr = flags & FMT_EXE_FLAG_PTR;
    signed char ch = ssint_arg_fetch(p_arg, ptr);
    if (mode == ARG_FETCH) rep = size_arg_fetch(p_arg, ptr);
    if (flags & FMT_EXE_FLAG_PHONY) return;
    if (mode == ARG_DEFAULT) rep = 1;
    if (rep <= *p_cnt) memset(buff, ch, rep * sizeof(buff));
    *p_cnt = rep;
}

static void fmt_execute_utf(uint32_t val, enum fmt_arg_mode mode, char *buff, size_t *p_cnt, Va_list *p_arg, enum fmt_execute_flags flags)
{
    if (mode == ARG_DEFAULT) return;
    if (mode == ARG_FETCH) val = uint32_arg_fetch(p_arg, flags & FMT_EXE_FLAG_PTR);
    if (flags & FMT_EXE_FLAG_PHONY) return;
    char str[UTF8_COUNT];
    uint8_t len;
    utf8_encode(val, str, &len);
    print(buff, p_cnt, str, len, 0);
}

static DECL_ARG_FETCH(fmt_callback, fmt_callback, fmt_callback)

static struct message_result fmt_execute(char *buff, size_t *p_cnt, void *p_arg, const struct style *style, enum fmt_execute_flags flags)
{
    const void *p_arg_list = NULL;
    void *p_arg_copy = NULL;
    bool ptr = flags & FMT_EXE_FLAG_PTR, list_ptr = 0;
    const char *fmt = ptr_arg_fetch(p_arg, ptr);
    if (!fmt)
    {
        if (!(flags & FMT_EXE_FLAG_PHONY)) *p_cnt = 0;
        return MESSAGE_SUCCESS;
    }
    Va_list arg_sub;
    enum fmt_execute_flags execute_flags = 0;
    size_t cnt = 0;
    const struct env *env;
    struct fmt_result res = { 0 };
    struct message_result message_res = MESSAGE_SUCCESS;
    for (size_t i = 0, len = *p_cnt, tmp = len, pos = 0, off = 0; ++i;) switch (i)
    {
    case 1:
        off = Strchrnull(fmt + pos, '%');
        if (fmt[pos + off])
        {
            print(buff + cnt, &len, fmt + pos, off, 0);
            pos += off + 1;
            off = cnt = add_sat(cnt, len);
            tmp = len = sub_sat(tmp, len);
            if (!tmp) i = SIZE_MAX; // Exit the loop if there is no more space
            break;
        }
        print(buff + cnt, &len, fmt + pos, off, 1);
        cnt = add_sat(cnt, len);
        i = SIZE_MAX; // Exit loop if there are no more arguments
        break;
    case 2:
        message_res = fmt_decode(&res, fmt, &pos);
        if (!message_res.status) return message_res;
        execute_flags = flags;
        list_ptr = res.flags & FMT_LIST_PTR;
        if (list_ptr)
        {
            p_arg_list = ptr_arg_fetch(p_arg, ptr);
            p_arg_copy = (void *) &p_arg_list; // Here MSVC promts warning which is supressed by '(void *)'
            execute_flags |= FMT_EXE_FLAG_PTR;
            list_ptr = list_ptr || ptr;
        }
        else if (res.flags & FMT_ARG_PTR)
        {
            Va_copy(arg_sub, *Va_list_ptr_arg_fetch(p_arg, ptr));
            p_arg_copy = &arg_sub;
        }
        else
        {
            p_arg_copy = p_arg;
            list_ptr = ptr;
        }
        //list_copy = res.flags & FMT_COPY_ARGS;
        //if (res.flags & FMT_COPY_ARGS) execute_flags |= FMT_EXE_FLAG_COPY_ARG;
        if ((res.flags & FMT_CONDITIONAL) && (!bool_arg_fetch(p_arg_copy, list_ptr) == !(res.flags & FMT_CONDITIONAL_NOT))) execute_flags |= FMT_EXE_FLAG_PHONY;
        if (res.flags & FMT_ENV_USER) env = env_ptr_arg_fetch(p_arg_copy, list_ptr);
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
        case TYPE_CODE_METRIC:
            env = &style->type_code_metric;
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
        int quote = -1;
        if (res.flags & FMT_QUOTE_SINGLE) quote = 0;
        else if (res.flags & FMT_QUOTE_DOUBLE) quote = 1;
        else if (res.flags & FMT_QUOTE_SELECT) quote = int_arg_fetch(p_arg_copy, list_ptr);
        const struct strl lquote[] = { STRI(UTF8_LSQUO), STRI(UTF8_LDQUO) }, rquote[] = { STRI(UTF8_RSQUO), STRI(UTF8_RDQUO) };
        quote = CLAMP(quote, -1, (int) MIN(countof(lquote), countof(rquote)));
        switch (res.type)
        {
        case TYPE_FMT:
        case TYPE_TIME_DIFF:
        case TYPE_CODE_METRIC:
        case TYPE_CALLBACK:
            for (size_t j = 0; ++j;) switch (j)
            {
            case 1:
                switch (res.type)
                {
                case TYPE_FMT:
                    message_res = fmt_execute(buff + cnt, &len, p_arg_copy, style, execute_flags);
                    break;
                case TYPE_TIME_DIFF:
                    message_res = fmt_execute_time_diff(buff + cnt, &len, p_arg_copy, env, execute_flags);
                    break;
                case TYPE_CODE_METRIC:
                    message_res = fmt_execute_code_metric(buff + cnt, &len, p_arg_copy, env, execute_flags);
                    break;
                case TYPE_CALLBACK:
                    message_res = fmt_callback_arg_fetch(p_arg_copy, list_ptr)(buff + cnt, &len, p_arg_copy, env, execute_flags);
                    break;
                default:
                    if (!(execute_flags & FMT_EXE_FLAG_PHONY)) j++;
                }
                if (!message_res.status) j = SIZE_MAX; // Exit on error
                else if (execute_flags & FMT_EXE_FLAG_PHONY) j++;
                break;
            case 2:
                cnt = add_sat(cnt, len);
                tmp = len = sub_sat(tmp, len);
            case 3:
                j = SIZE_MAX;
            }
            break;
        default:
            for (size_t j = 0; ++j;) switch (j)
            {
            case 2:
            case 4:
            case 6:
            case 8:
                cnt = add_sat(cnt, len);
                tmp = len = sub_sat(tmp, len);
                if (!tmp) j = SIZE_MAX;
                break;            
            case 1:
                if (quote >= 0 && !(execute_flags & FMT_EXE_FLAG_PHONY)) print(buff + cnt, &len, STRL(lquote[quote]), 0);
                else j++;
                break;
            case 3:
                if (env && !(execute_flags & FMT_EXE_FLAG_PHONY)) print(buff + cnt, &len, STRL(env->begin), 0);
                else j++;
                break;
            case 5:
                switch (res.type)
                {
                case TYPE_INT:
                    message_res = fmt_execute_int(res.int_arg.spec, res.int_arg.flags, buff + cnt, &len, p_arg_copy, execute_flags);
                    break;
                case TYPE_STR:
                case TYPE_PATH:
                    fmt_execute_str(res.str_arg.len, res.str_arg.mode, buff + cnt, &len, p_arg_copy, execute_flags);
                    break;
                case TYPE_TIME_STAMP:
                    if (!(execute_flags & FMT_EXE_FLAG_PHONY)) print_time_stamp(buff + cnt, &len);
                    break;
                case TYPE_CRT:
                    fmt_execute_crt(buff + cnt, &len, p_arg_copy, execute_flags);
                    break;
                case TYPE_DEFAULT:
                    fmt_execute_default(res.str_arg.len, res.str_arg.mode, &len, p_arg_copy, execute_flags);
                    break;
                case TYPE_CHAR:
                    fmt_execute_char(res.char_arg.rep, res.char_arg.mode, buff + cnt, &len, p_arg_copy, execute_flags);
                    break;
                case TYPE_PERC:
                    if (!(execute_flags & FMT_EXE_FLAG_PHONY)) print(buff + cnt, &len, "%", 1, 0);
                    break;
                case TYPE_UTF:
                    fmt_execute_utf(res.utf_arg.val, res.utf_arg.mode, buff + cnt, &len, p_arg_copy, execute_flags);
                    break;
                case TYPE_FLT: // NOT IMPLEMENTED!
                default:
                    if (!(execute_flags & FMT_EXE_FLAG_PHONY)) j++;
                }
                if (!message_res.status) j = SIZE_MAX; // Exit on error
                else if (execute_flags & FMT_EXE_FLAG_PHONY) j++;
                break;
            case 7:
                if (env && !(execute_flags & FMT_EXE_FLAG_PHONY)) print(buff + cnt, &len, STRL(env->end), 0);
                else j++;
                break;
            case 9:
                if (quote >= 0 && !(execute_flags & FMT_EXE_FLAG_PHONY)) print(buff + cnt, &len, STRL(rquote[quote]), 0);
                else j++;
                break;
            case 10:
                cnt = add_sat(cnt, len);
                tmp = len = sub_sat(tmp, len);
            case 11:
                j = SIZE_MAX;
            } 
        }
        if ((res.flags & FMT_ARG_PTR) && !(res.flags & FMT_LIST_PTR)) Va_end(arg_sub);
        if (!message_res.status) return message_res;
        if (!tmp) i = SIZE_MAX;
        break;
    case 3:
        if (res.flags & FMT_LEFT_JUSTIFY)
        {
            size_t diff = cnt - off, disp = sub_sat(off, diff);
            memmove(buff + disp, buff + off, diff * sizeof(*buff));
            cnt = res.flags & FMT_OVERPRINT ? disp : disp + diff;
            tmp = len = *p_cnt - cnt;
        }
        else if (res.flags & FMT_OVERPRINT) tmp = len = *p_cnt - (cnt = off);
        i = 0; // Return to beginning
    }
    if (!(flags & FMT_EXE_FLAG_PHONY)) *p_cnt = cnt;
    return MESSAGE_SUCCESS;
}

struct message_result print_fmt(char *buff, size_t *p_cnt, const struct style *style, ...)
{
    Va_list arg;
    Va_start(arg, style);
    struct message_result res = fmt_execute(buff, p_cnt, &arg, style, 0);
    Va_end(arg);
    return res;
}

// Last argument may be 'NULL'
bool log_init(struct log *restrict log, const char *restrict path, size_t hint, enum log_flags flags, const struct ttl_style *restrict ttl_style, const struct style *restrict style, struct log *restrict fallback)
{
    FILE *f = path ? Fopen(path, flags & LOG_APPEND ? "a" : "w") : Fdup(stderr, "a");
    if (!fopen_assert(fallback, CODE_METRIC, path, f)) return 0;
    if (!setvbuf(f, NULL, _IONBF, 0))
    {
        log->mutex = NULL;
        if (!(flags & LOG_WRITE_LOCK) || (
            array_assert(fallback, CODE_METRIC, array_init(&log->mutex, NULL, 1, sizeof(*log->mutex), 0, ARRAY_FAILSAFE)) &&
            thread_assert(fallback, CODE_METRIC, mutex_init(log->mutex))))
        {
            bool tty = (flags & (LOG_FORCE_TTY)) || Fisatty(f), bom = !(tty || (flags & (LOG_NO_BOM | LOG_APPEND)));
            size_t cap = bom ? MAX(hint, lengthof(UTF8_BOM)) : hint;
            if (array_assert(fallback, CODE_METRIC, array_init(&log->buff, &log->cap, cap, sizeof(*log->buff), 0, 0)))
            {
                if (bom) memcpy(log->buff, UTF8_BOM, (log->cnt = lengthof(UTF8_BOM)) * sizeof(*log->buff));
                else log->cnt = 0;
                log->ttl_style = tty ? ttl_style : NULL;
                log->style = tty ? style : NULL;
                log->hint = hint;
                log->file = f;
                log->tot = 0;
                log->fallback = fallback;
                if (log->cnt <= log->hint || log_flush(log, 0)) return 1;
                free(log->buff);
            }
            mutex_close(log->mutex);
        }
        free(log->mutex);
    }
    Fclose(f);
    return 0;
}

bool log_mirror_init(struct log *restrict dst, struct log *restrict src)
{
    if (!log_flush(src, 0) ||
        !array_assert(src, CODE_METRIC, array_init(&dst->buff, &dst->cap, src->hint, sizeof(*dst->buff), 0, 0))) return 0;
    dst->cnt = 0;
    dst->ttl_style = src->ttl_style;
    dst->style = src->style;
    dst->hint = src->hint;
    dst->file = src->file;
    dst->tot = 0;
    dst->mutex = src->mutex;
    dst->fallback = src->fallback;
    return 1;
}

bool log_flush(struct log *restrict log, bool lock)
{
    size_t cnt = log->cnt;
    if (!cnt) return 1;
    if (lock && log->mutex) mutex_acquire(log->mutex);
    size_t wr = Fwrite_unlocked(log->buff, sizeof(*log->buff), cnt, log->file);
    int res = Fflush_unlocked(log->file);
    if (lock && log->mutex) mutex_release(log->mutex);
    log->tot += wr;
    log->cnt = 0;
    return wr == cnt && crt_assert(log->fallback, CODE_METRIC, !res);
}

void log_close(struct log *restrict log)
{
    if (!log) return;
    log_flush(log, 0);
    Fclose(log->file);
    mutex_close(log->mutex);
    free(log->mutex);
    free(log->buff);
}

void log_mirror_close(struct log *restrict log)
{
    if (!log) return;
    log_flush(log, 0);
    free(log->buff);
}

static struct message_result log_prefix(char *buff, size_t *p_cnt, struct code_metric metric, enum message_type type, const struct ttl_style *ttl_style)
{
    const struct strl ttl[] = { STRI("MESSAGE"), STRI("ERROR"), STRI("WARNING"), STRI("NOTE"), STRI("INFO"), STRI("DAMNATION") };
    return ttl_style ?
        print_fmt(buff, p_cnt, NULL, "%s*[%D]%s* %~~s* %s*(%M):%s* ",
            STRL(ttl_style->time_stamp.begin),
            STRL(ttl_style->time_stamp.end),
            ttl_style->header + type, STRL(ttl[type]),
            STRL(ttl_style->code_metric.begin),
            metric,
            STRL(ttl_style->code_metric.end)) :
        print_fmt(buff, p_cnt, NULL, "[%D] %s* (%M): ", STRL(ttl[type]), metric);
}

static bool log_fallback(struct log *log, struct code_metric metric_src, struct code_metric metric_dst)
{
    return log_message_fmt(log, metric_dst, MESSAGE_ERROR, "Unable to display message issued from the function %~M!\n", metric_src);
}

static bool log_message_impl(struct log *restrict log, struct code_metric metric, enum message_type type, union message_callback handler, void *context, Va_list *p_arg)
{
    if (!log) return 1;
    if (!log->fallback && log->mutex) mutex_acquire(log->mutex);
    size_t cnt = log->cnt;
    for (unsigned i = 0; i < 2; i++) for (;;)
    {
        size_t len = log->cap - cnt;
        struct message_result message_res = MESSAGE_SUCCESS;
        switch (i)
        {
        case 0:
            message_res = log_prefix(log->buff + cnt, &len, metric, type, log->ttl_style);
            break;
        case 1:
            if (p_arg)
            {
                Va_list arg;
                Va_copy(arg, *p_arg);
                message_res = handler.var(log->buff + cnt, &len, context, log->style, arg);
                Va_end(arg);
            }
            else message_res = handler.ord(log->buff + cnt, &len, context, log->style);
        }
        if (!message_res.status)
        {
            log_fallback(log->fallback, metric, message_res.metric);
            return 0;
        }
        struct array_result array_res; // All messages are NULL-terminated
        if (!array_assert(log->fallback, CODE_METRIC, array_res = array_test(&log->buff, &log->cap, sizeof(*log->buff), 0, 0, cnt, len, 1))) return 0;
        if (!(array_res.status & ARRAY_UNTOUCHED)) continue;
        cnt += len;
        break;
    }
    log->cnt = cnt;
    bool res =  cnt > log->hint ? log_flush(log, log->fallback) : 1;
    if (!log->fallback && log->mutex) mutex_release(log->mutex);
    return res;
}

bool log_message(struct log *restrict log, struct code_metric code_metric, enum message_type type, message_callback handler, void *context)
{
    return log_message_impl(log, code_metric, type, (union message_callback) { .ord = handler }, context, NULL);
}

static struct message_result message_var(char *buff, size_t *p_cnt, void *context, const struct style *style, Va_list arg)
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

// To be removed
bool log_message_crt(struct log *restrict log, struct code_metric code_metric, enum message_type type, Errno_t err)
{
    return log_message_fmt(log, code_metric, type, "%C!\n", err);
}

// To be removed
bool log_message_fopen(struct log *restrict log, struct code_metric code_metric, enum message_type type, const char *restrict path, Errno_t err)
{
    return log_message_fmt(log, code_metric, type, "Unable to open the file %\"~P. %C!\n", path, err);
}

bool log_message_fseek(struct log *restrict log, struct code_metric code_metric, enum message_type type, int64_t offset, const char *restrict path)
{
    return log_message_fmt(log, code_metric, type, "Unable to seek into the position %~uq of the file %\"~P!\n", offset, path);
}

bool fopen_assert(struct log *restrict log, struct code_metric code_metric, const char *restrict path, bool res)
{
    if (!res) log_message_fmt(log, code_metric, MESSAGE_ERROR, "Unable to open the file %\"~P. %C!\n", path, errno);
    return res;
}

bool array_assert(struct log *log, struct code_metric code_metric, struct array_result res)
{
    switch (res.error)
    {
    case ARRAY_OUT_OF_MEMORY:
        log_message_fmt(log, code_metric, MESSAGE_ERROR, "Request for %~uz bytes of memory denied. %C!\n", res.tot, errno);
        break;
    case ARRAY_OVERFLOW:
        log_message_fmt(log, code_metric, MESSAGE_ERROR, "Memory allocation impossible. %C!\n", ERANGE);
        break;
    case ARRAY_NO_ERROR:
        break;
    }
    return res.status;
}

bool crt_assert_impl(struct log *log, struct code_metric code_metric, Errno_t err)
{
    if (err) log_message_fmt(log, code_metric, MESSAGE_ERROR, "%C!\n", err);
    return !err;
}

bool crt_assert(struct log *log, struct code_metric code_metric, bool res)
{
    return res || crt_assert_impl(log, code_metric, errno);
}

#ifdef _WIN32

#   include <windows.h>

static struct message_result fmt_execute_wstr(char *buff, size_t *p_cnt, void *p_arg, const struct env *env, enum fmt_execute_flags flags)
{
    (void) env;
    bool ptr = flags & FMT_EXE_FLAG_PTR;
    const wchar_t *wstr = ptr_arg_fetch(p_arg, ptr);
    size_t wlen = size_arg_fetch(p_arg, ptr);
    if (flags & FMT_EXE_FLAG_PHONY) return MESSAGE_SUCCESS;
    uint32_t val;
    uint8_t context = 0;
    char str[UTF8_COUNT];
    size_t len = 0, cnt = *p_cnt;
    for (size_t i = 0; i < wlen; i++)
    {
        if (!utf16_decode((uint16_t) wstr[i], &val, NULL, NULL, &context, 0)) return MESSAGE_FAILURE;
        if (context) continue;
        uint8_t ulen;
        utf8_encode(val, str, &ulen);
        size_t tmp = cnt;
        print(buff + len, &tmp, str, ulen, 0);
        len = add_sat(len, tmp);
        cnt = sub_sat(cnt, tmp);
        if (!cnt) break;
    }
    if (context) return MESSAGE_FAILURE;
    *p_cnt = len;
    return MESSAGE_SUCCESS;
}

static struct message_result fmt_execute_wapi(char *buff, size_t *p_cnt, void *p_arg, const struct env *env, enum fmt_execute_flags flags)
{
    (void) env;
    uint32_t err = uint32_arg_fetch(p_arg, flags & FMT_EXE_FLAG_PTR);
    if (flags & FMT_EXE_FLAG_PHONY) return MESSAGE_SUCCESS;
    wchar_t *wstr = NULL;
    size_t wlen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, (DWORD) err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR) &wstr, 0, NULL);
    if (!wstr) return MESSAGE_FAILURE;
    struct message_result res = print_fmt(buff, p_cnt, NULL, "%&", fmt_execute_wstr, (void *) wstr, sub_sat(wlen, lengthof(".\r\n"))); // Cutting ".\r\n" at the end of the string
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
    log_message_fmt(log, code_metric, MESSAGE_RESERVED, "Function %''~s* should be called only when %''~s* macro is defined!\n", STRC(__func__), STRC("_WIN32"));
    return res;
}

#endif
