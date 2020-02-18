#include "np.h"
#include "ll.h"
#include "argv.h"
#include "memory.h"
#include "sort.h"
#include "utf8.h"

#include <string.h>
#include <stdlib.h>

static bool log_message_warning_generic(struct log *restrict log, struct code_metric code_metric, size_t ind, ...)
{
    Va_list arg;
    Va_start(arg, ind);
    bool res = log_message_fmt(log, code_metric, MESSAGE_WARNING, "%@$ the command-line argument no. %~uz!\n", &arg, ind);
    Va_end(arg);
    return res;
}

enum argv_name_val_status {
    ARGV_UNHANDLED_PAR = 0,
    ARGV_UNEXPECTED_VALUE
};

static bool log_message_warning_argv_name_val(struct log *restrict log, struct code_metric code_metric, size_t ind, enum argv_name_val_status status, bool shrt, const char *name_str, size_t name_len, const char *val_str, size_t val_len)
{
    const char *fmt[] = { 
        "Unable to handle the value %~s* of the parameter %~'s* near",
        "Redundant value %~s* for the option %~'s* within"
    };
    return log_message_warning_generic(log, code_metric, ind, fmt[status], val_str, val_len, !shrt, name_str, name_len);
}

enum argv_name_status {
    ARGV_UNHANDLED_OPT = 0,
    ARGV_INVALID_PAR,
    ARGV_MISSING_VALUE
};

static bool log_message_warning_argv_name(struct log *restrict log, struct code_metric code_metric, size_t ind, enum argv_name_status status, bool shrt, const char *name_str, size_t name_len)
{
    const char *fmt[] = {
        "Unable to handle the option %~'s* within",
        "Invalid identifier %~'s* within",
        "Expected a value for the parameter %~'s* within"
    };
    return log_message_warning_generic(log, code_metric, ind, fmt[status], !shrt, name_str, name_len);
}

static bool log_message_warning_argv_utf(struct log *restrict log, struct code_metric code_metric, size_t ind, size_t pos)
{
    return log_message_warning_generic(log, code_metric, ind, "Unexpected UTF-8 continuation byte at the position %~uz within", pos + 1);
}

bool argv_parse(par_selector_callback selector, void *context, void *res, char **argv, size_t argc, char ***p_arr, size_t *p_cnt, struct log *log)
{
    struct par par = { 0 };
    char *str = NULL;
    size_t cnt = 0, len = 0;
    bool halt = 0, succ = 1;
    int capture = 0;
    *p_arr = NULL;
    *p_cnt = 0;
    for (size_t i = 1; i < argc; i++)
    {
        if (!halt)
        {
            bool hyph = argv[i][0] == '-';
            if (capture)
            {
                bool shrt = capture < 0;
                capture = 0;
                if (!(par.mode == PAR_VALUED_OPTION && hyph))
                {
                    size_t tmp = strlen(argv[i]);
                    if (par.handler && !par.handler(argv[i], tmp, par.ptr, par.context)) log_message_warning_argv_name_val(log, CODE_METRIC, i - 1, ARGV_UNHANDLED_PAR, shrt, str, len, argv[i], tmp);
                    continue;
                }
                else if (par.handler && !par.handler(NULL, 0, par.ptr, par.context)) log_message_warning_argv_name(log, CODE_METRIC, i, ARGV_UNHANDLED_OPT, shrt, str, len);
            }
            if (hyph)
            {
                if (argv[i][1] == '-') // Long mode
                {
                    str = argv[i] + 2;
                    if (*str)
                    {
                        len = Strchrnul(str, '=');
                        if (!selector(&par, str, len, res, context, 0)) log_message_warning_argv_name(log, CODE_METRIC, i, ARGV_INVALID_PAR, 0, str, len);
                        else
                        {
                            if (par.mode == PAR_OPTION)
                            {
                                if (str[len]) log_message_warning_argv_name_val(log, CODE_METRIC, i, ARGV_UNEXPECTED_VALUE, 0, str, len, str + len + 1, strlen(str + len + 1));
                                if (par.handler && !par.handler(NULL, 0, par.ptr, par.context)) log_message_warning_argv_name(log, CODE_METRIC, i, ARGV_UNHANDLED_OPT, 0, str, len);
                            }
                            else if (!str[len]) capture = 1;
                            else
                            {
                                size_t tmp = strlen(str + len + 1);
                                if (par.handler && !par.handler(str + len + 1, tmp, par.ptr, par.context)) log_message_warning_argv_name_val(log, CODE_METRIC, i, ARGV_UNHANDLED_PAR, 0, str, len, str + len + 1, tmp);
                            }
                        }
                    }
                    else halt = 1; // halt on "--"
                }
                else // Short mode
                {
                    size_t tot = strlen(argv[i] + 1);
                    for (size_t k = 1; argv[i][k];) // Inner loop for handling multiple option-like parameters
                    {
                        str = argv[i] + k;
                        if (!utf8_decode_len(str, tot, &len)) log_message_warning_argv_utf(log, CODE_METRIC, i, k + len);
                        else
                        {
                            if (!selector(&par, str, len, res, context, 1)) log_message_warning_argv_name(log, CODE_METRIC, i, ARGV_INVALID_PAR, 1, str, len);
                            else
                            {
                                if (par.mode != PAR_OPTION) // Parameter expects value
                                {
                                    if (!str[len]) capture = -1;
                                    else
                                    {
                                        size_t tmp = strlen(str + len);
                                        if (par.handler && !par.handler(str + len, tmp, par.ptr, par.context)) log_message_warning_argv_name_val(log, CODE_METRIC, i, ARGV_UNHANDLED_PAR, 1, str, len, str + len, tmp);
                                    }
                                    break; // Exiting from the inner loop
                                }
                                else if (par.handler && !par.handler(NULL, 0, par.ptr, par.context)) log_message_warning_argv_name(log, CODE_METRIC, i, ARGV_UNHANDLED_OPT, 1, str, len);
                            }
                            if ((k += len) > tot) break;
                        }
                    }
                }
                continue;
            }            
        }
        if (array_assert(log, CODE_METRIC, array_test(p_arr, p_cnt, sizeof(**p_arr), 0, 0, cnt, 1)))
        {
            (*p_arr)[cnt++] = argv[i]; // Storing positional parameters
            continue;
        }
        succ = 0;
        break;
    }
    if (succ)
    {
        if (capture && par.mode == PAR_VALUED) log_message_warning_argv_name(log, CODE_METRIC, argc - 1, ARGV_MISSING_VALUE, capture < 0, str, len);
        array_test(p_arr, p_cnt, sizeof(**p_arr), 0, ARRAY_REDUCE, cnt);
        return 1;
    }
    free(*p_arr);
    *p_cnt = 0;
    return 0;    
}

bool argv_par_selector(struct par *par, const char *str, size_t len, void *res, void *Context, bool shrt)
{
    struct argv_par_sch *context = Context;
    struct tag *tag = NULL;
    size_t cnt = 0, ind;
    if (shrt) tag = context->stag, cnt = context->stag_cnt;
    else tag = context->ltag, cnt = context->ltag_cnt;
    if (binary_search(&ind, str, tag, cnt, sizeof(*tag), str_strl_stable_cmp, &len, 0))
    {
        size_t id = tag[ind].id;
        if (id < context->par_sch_cnt)
        {
            struct par_sch par_sch = context->par_sch[id];
            *par = (struct par) { .ptr = (char *) res + par_sch.off, .context = par_sch.context, .handler = par_sch.handler, .mode = par_sch.mode };
            return 1;
        }
    }
    return 0;
}
