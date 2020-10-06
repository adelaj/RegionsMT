#include "np.h"
#include "main.h"
#include "argv.h"
#include "lm.h"
#include "memory.h"
#include "utf8.h"

#include "threadpool.h"

#include "lm.h"

#include "module/categorical.h"
#include "module/lde.h"

#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <float.h>

#include <gsl/gsl_sf.h>
#include "gslsupp.h"

#include "sort.h"

//#include <windows.h>

#ifndef TEST_DEACTIVATE

#   include "test.h"
#   include "test/categorical.h"
#   include "test/ll.h"
#   include "test/np.h"
#   include "test/sort.h"
#   include "test/utf8.h"

static bool test_main(size_t thread_cnt, struct log *log)
{
    const struct test_group groupl[] = {
        {
            // Low level facilities A
            test_ll_dispose_a,
            CLII((struct test_generator []) {
                FSTRL(test_ll_generator_a)
            }),
            CLII((struct test []) {
                FSTRL(test_ll_a_1),
                FSTRL(test_ll_a_2),
                FSTRL(test_ll_a_3)
            })
        },
        {
            // Low level facilities B
            test_ll_dispose_b,
            CLII((struct test_generator []) {
                FSTRL(test_ll_generator_b),
            }),
            CLII((struct test []) {
                FSTRL(test_ll_b),
            })
        },
        {
            // Emulation of the non-portable facilities
            test_np_dispose_a,
            CLII((struct test_generator []) {
                FSTRL(test_np_generator_a),
            }),
            CLII((struct test []) {
                FSTRL(test_np_a),
            })
        },
        {
            // Sorting routines A: sort algorithms
            test_sort_dispose_a,
            CLII((struct test_generator []) {
                FSTRL(test_sort_generator_a_1),
                FSTRL(test_sort_generator_a_2),
                FSTRL(test_sort_generator_a_3)
            }),
            CLII((struct test []) {
                FSTRL(test_sort_a),
            })
        },
        {
            // Sorting routines B: order computation
            test_sort_dispose_b,
            CLII((struct test_generator []) {
                FSTRL(test_sort_generator_b_1),
            }),
            CLII((struct test []) {
                FSTRL(test_sort_b_1),
                FSTRL(test_sort_b_2)
            })
        },
        {
            // Sorting routines C: precise search
            test_sort_dispose_c,
            CLII((struct test_generator []) {
                FSTRL(test_sort_generator_c_1)
            }),
            CLII((struct test []) {
                FSTRL(test_sort_c_1),
                FSTRL(test_sort_c_2)
            })
        },
        {
            // Sorting routines D: approximate search
            test_sort_dispose_d,
            CLII((struct test_generator []) {
                FSTRL(test_sort_generator_d_1)
            }),
            CLII((struct test []) {
                FSTRL(test_sort_d_1)
            })
        },
        {
            // Unicode facilities
            test_utf8_dispose,
            CLII((struct test_generator []) {
                FSTRL(test_utf8_generator),
            }),
            CLII((struct test []) {
                FSTRL(test_utf8_len),
                FSTRL(test_utf8_encode),
                FSTRL(test_utf8_decode),
                FSTRL(test_utf16_encode),
                FSTRL(test_utf16_decode)
            })
        },
        {
            // Categorical data analysis
            test_categorical_dispose_a,
            CLII((struct test_generator []) {
                FSTRL(test_categorical_generator_a),
            }),
            CLII((struct test []) {
                FSTRL(test_categorical_a),
            })
        }
    };
    log_message_fmt(log, CODE_METRIC, MESSAGE_NOTE, "Test mode triggered.\n");
    return test(groupl, countof(groupl), thread_cnt, log);
}

#else

static bool test_main(struct fallback *fallback)
{
    log_message_fmt(fallback, CODE_METRIC, MESSAGE_NOTE, "Test mode deactivated!\n");
    return 1;
}

#endif

// Main routine arguments management
struct main_args main_args_default()
{
    struct main_args res = { .thread_cnt = get_processor_count() };
    uint8_bit_set(res.bits, MAIN_ARGS_BIT_POS_THREAD_CNT);
    return res;
}

struct main_args main_args_override(struct main_args args_hi, struct main_args args_lo)
{
    struct main_args res = { .log_path = args_hi.log_path ? args_hi.log_path : args_lo.log_path };
    memcpy(res.bits, args_hi.bits, UINT8_CNT(MAIN_ARGS_BIT_CNT));
    if (uint8_bit_test(args_hi.bits, MAIN_ARGS_BIT_POS_THREAD_CNT)) res.thread_cnt = args_hi.thread_cnt;
    else
    {
        res.thread_cnt = args_lo.thread_cnt;
        uint8_bit_set(res.bits, MAIN_ARGS_BIT_POS_THREAD_CNT);
    }
    return res;
}

// Main routine (see below for actual entry point)
static int Main(int argc, char **argv) 
{
    /*const char *strings[] = {
        __FUNCTION__,
        "NOTE (%s): No input data specified.\n",
        "INFO (%s): Help mode triggered.\n",
        "WARNING (%s): Unable to compile XML document \"%s\"!\n",
        "WARNING (%s): Unable to execute one or more branches of the XML document \"%s\"!\n",
        "Overall program execution"
    };

    enum
    {
        STR_FN = 0,
        STR_FR_ND,
        STR_FR_IH,
        STR_FR_WC,
        STR_FR_WE,
        STR_M_EXE
    };   
    */

    // All names should be sorted according to 'strncmp'!!!
    struct argv_par_sch argv_par_sch =
    {
        CLII((struct tag[]) { { STRI("fancy"), 7 }, { STRI("help"), 0 }, { STRI("log"), 1 }, { STRI("test"), 2 }, { STRI("threads"), 3 }}),
        CLII((struct tag[]) { { STRI("C"), 4 }, { STRI("F"), 6 }, { STRI("L"), 5 }, { STRI("T"), 2 }, { STRI("h"), 0 }, { STRI("l"), 1 }, { STRI("m"), 10 }, { STRI("t"), 3 } }),
        CLII((struct par_sch[])
        {
            [0] = { 0, &(struct handler_context) { offsetof(struct main_args, bits), MAIN_ARGS_BIT_POS_HELP }, empty_handler, PAR_OPTION },
            [1] = { offsetof(struct main_args, log_path), NULL, p_str_handler, PAR_VALUED_OPTION },
            [2] = { 0, &(struct handler_context) { offsetof(struct main_args, bits), MAIN_ARGS_BIT_POS_TEST }, empty_handler, PAR_OPTION },
            [3] = { offsetof(struct main_args, thread_cnt), &(struct handler_context) { offsetof(struct main_args, bits) - offsetof(struct main_args, thread_cnt), MAIN_ARGS_BIT_POS_THREAD_CNT }, size_handler, PAR_VALUED },
            [4] = { 0, &(struct handler_context) { offsetof(struct main_args, bits), MAIN_ARGS_BIT_POS_CAT }, empty_handler, PAR_OPTION },
            [5] = { 0, &(struct handler_context) { offsetof(struct main_args, bits), MAIN_ARGS_BIT_POS_LDE }, empty_handler, PAR_OPTION },
            [6] = { 0, &(struct handler_context) { offsetof(struct main_args, bits), MAIN_ARGS_BIT_POS_CHISQ }, empty_handler, PAR_OPTION },
            [7] = { offsetof(struct main_args, bits), &(struct bool_handler_context) { MAIN_ARGS_BIT_POS_FANCY, &(struct handler_context) { 0, MAIN_ARGS_BIT_POS_FANCY_USER } }, bool_handler2, PAR_VALUED_OPTION },
            [10] = { 0, &(struct handler_context) { offsetof(struct main_args, bits), MAIN_ARGS_BIT_POS_LM}, empty_handler, PAR_OPTION },
        })
    };

    /*
    
    // All names on every sub-level should be sorted according to 'strncmp'!!!
    struct { xmlNode *node; size_t cnt; } dscsch = CLII((xmlNode[])
    {
        {
            .name = STRI("Density"),
            .sz = sizeof(densityContext),
            .prologue = (prologueCallback) densityPrologue,
            .epilogue = (epilogueCallback) densityEpilogue,
            .dispose = (disposeCallback) densityContextDispose,
            CLII((struct att[])
            {
                { STRI("pos"), 0, &(handlerContext) { offsetof(densityContext, bits), DENSITYCONTEXT_BIT_POS_POS }, (readHandlerCallback) boolHandler },
                { STRI("radius"), offsetof(densityContext, radius), &(handlerContext) { offsetof(densityContext, bits) - offsetof(densityContext, radius), DENSITYCONTEXT_BIT_POS_RADIUS }, (readHandlerCallback) uint32Handler },
                { STRI("type"), offsetof(densityContext, type), NULL, (readHandlerCallback) densityTypeHandler }
            }),
            CLII((xmlNode[])
            {
                {
                    .name = STRI("Fold"),
                    .sz = sizeof(densityFoldContext),
                    .prologue = (prologueCallback) densityFoldPrologue,
                    .epilogue = (epilogueCallback) densityFoldEpilogue,
                    .dispose = (disposeCallback) densityFoldContextDispose,
                    CLII((struct att[])
                    {
                        { STRI("group"), offsetof(densityFoldContext, group), NULL, (readHandlerCallback) strHandler },
                        { STRI("type"), offsetof(densityFoldContext, type), NULL, (readHandlerCallback) densityFoldHandler }
                    }),
                    CLII((xmlNode[])
                    {
                        {
                            .name = STRI("Report"),
                            .sz = sizeof(dfReportContext),
                            .prologue = (prologueCallback) dfReportPrologue,
                            .epilogue = (epilogueCallback) dfReportEpilogue,
                            .dispose = (disposeCallback) dfReportContextDispose,
                            CLII((struct att[])
                            {
                                { STRI("header"), 0, &(handlerContext) { offsetof(dfReportContext, bits), DFREPORTCONTEXT_BIT_POS_HEADER }, (readHandlerCallback) boolHandler },
                                { STRI("limit"), offsetof(dfReportContext, limit), &(handlerContext) { offsetof(dfReportContext, bits) - offsetof(dfReportContext, limit), DFREPORTCONTEXT_BIT_POS_LIMIT }, (readHandlerCallback) uint32Handler },
                                { STRI("path"), offsetof(dfReportContext, path), NULL, (readHandlerCallback) strHandler },
                                { STRI("semicolon"), 0, &(handlerContext) { offsetof(dfReportContext, bits), DFREPORTCONTEXT_BIT_POS_SEMICOLON }, (readHandlerCallback) boolHandler },
                                { STRI("threshold"), offsetof(dfReportContext, threshold), &(handlerContext) { offsetof(dfReportContext, bits) - offsetof(dfReportContext, threshold), DFREPORTCONTEXT_BIT_POS_THRESHOLD }, (readHandlerCallback) float64Handler },
                                { STRI("type"), offsetof(dfReportContext, type), NULL, (readHandlerCallback) dfReportHandler }
                            })
                        }
                    })
                },
                {
                    .name = STRI("Regions"),
                    .sz = sizeof(regionsContext),
                    .prologue = (prologueCallback) regionsPrologue,
                    .epilogue = (epilogueCallback) regionsEpilogue,
                    .dispose = (disposeCallback) regionsContextDispose,
                    CLII((struct att[])
                    {
                        { STRI("decay"), offsetof(regionsContext, decay), &(handlerContext) { offsetof(regionsContext, bits) - offsetof(regionsContext, decay), REGIONSCONTEXT_BIT_POS_DECAY }, (readHandlerCallback) uint32Handler },
                        { STRI("depth"), offsetof(regionsContext, depth), &(handlerContext) { offsetof(regionsContext, bits) - offsetof(regionsContext, depth), REGIONSCONTEXT_BIT_POS_DEPTH }, (readHandlerCallback) uint32Handler },
                        { STRI("length"), offsetof(regionsContext, length), &(handlerContext) { offsetof(regionsContext, bits) - offsetof(regionsContext, length), REGIONSCONTEXT_BIT_POS_LENGTH }, (readHandlerCallback) uint32Handler },
                        { STRI("threshold"), offsetof(regionsContext, threshold), &(handlerContext) { offsetof(regionsContext, bits) - offsetof(regionsContext, threshold), REGIONSCONTEXT_BIT_POS_THRESHOLD }, (readHandlerCallback) float64Handler },
                        { STRI("tolerance"), offsetof(regionsContext, tolerance), &(handlerContext) { offsetof(regionsContext, bits) - offsetof(regionsContext, tolerance), REGIONSCONTEXT_BIT_POS_TOLERANCE }, (readHandlerCallback) float64Handler },
                        { STRI("slope"), offsetof(regionsContext, slope), &(handlerContext) { offsetof(regionsContext, bits) - offsetof(regionsContext, slope), REGIONSCONTEXT_BIT_POS_SLOPE }, (readHandlerCallback) float64Handler }
                    })
                },
                {
                    .name = STRI("Report"),
                    .sz = sizeof(dReportContext),
                    .prologue = (prologueCallback) dReportPrologue,
                    .epilogue = (epilogueCallback) dReportEpilogue,
                    .dispose = (disposeCallback) dReportContextDispose,
                    CLII((struct att[])
                    {
                        { STRI("header"), 0, &(handlerContext) { offsetof(dReportContext, bits), DREPORTCONTEXT_BIT_POS_HEADER }, (readHandlerCallback) boolHandler },
                        { STRI("limit"), offsetof(dReportContext, limit), &(handlerContext) { offsetof(dReportContext, bits) - offsetof(dReportContext, limit), DREPORTCONTEXT_BIT_POS_LIMIT }, (readHandlerCallback) uint32Handler },
                        { STRI("path"), offsetof(dReportContext, path), NULL, (readHandlerCallback) strHandler },
                        { STRI("semicolon"), 0, &(handlerContext) { offsetof(dReportContext, bits), DREPORTCONTEXT_BIT_POS_SEMICOLON }, (readHandlerCallback) boolHandler },
                        { STRI("threshold"), offsetof(dReportContext, threshold), &(handlerContext) { offsetof(dReportContext, bits) - offsetof(dReportContext, threshold), DREPORTCONTEXT_BIT_POS_THRESHOLD }, (readHandlerCallback) float64Handler }
                    })
                }
            })
        },
        {
            .name = STRI("MySQL.Dispatch"),
            .sz = sizeof(mysqlDispatchContext),
            .prologue = (prologueCallback) mysqlDispatchPrologue,
            .epilogue = (epilogueCallback) mysqlDispatchEpilogue,
            .dispose = (disposeCallback) mysqlDispatchContextDispose,
            CLII((struct att[])
            {
                { STRI("host"), offsetof(mysqlDispatchContext, host), NULL, (readHandlerCallback) strHandler },
                { STRI("login"), offsetof(mysqlDispatchContext, login), NULL, (readHandlerCallback) strHandler },
                { STRI("password"), offsetof(mysqlDispatchContext, password), NULL, (readHandlerCallback) strHandler },
                { STRI("port"), offsetof(mysqlDispatchContext, port), NULL, (readHandlerCallback) uint32Handler },
                { STRI("schema"), offsetof(mysqlDispatchContext, schema), NULL, (readHandlerCallback) strHandler },
                { STRI("temp.prefix"), offsetof(mysqlDispatchContext, temppr), NULL, (readHandlerCallback) strHandler },
            })
        }
    });
    
    // All names on every sub-level should be sorted according to 'strncmp'!!!
    xmlNode xmlsch =
    {
        .name = STRI("RegionsMT"),
        .sz = sizeof(frameworkContext),
        .prologue = (prologueCallback) frameworkPrologue,
        .epilogue = (epilogueCallback) frameworkEpilogue,
        .dispose = (disposeCallback) frameworkContextDispose,
        CLII((struct att[])
        {
            { STRI("log"), offsetof(frameworkContext, logFile), NULL, (readHandlerCallback) strHandler },
            { STRI("threads"), offsetof(frameworkContext, threadCount), &(handlerContext) { offsetof(frameworkContext, bits) - offsetof(frameworkContext, threadCount), FRAMEWORKCONTEXT_BIT_POS_THREADCOUNT }, (readHandlerCallback) uint32Handler }
        }),
        CLII((xmlNode[])
        {
            {
                .name = STRI("Data.Load"),
                .sz = sizeof(loadDataContext),
                .prologue = (prologueCallback) loadDataPrologue,
                .epilogue = (epilogueCallback) loadDataEpilogue,
                .dispose = (disposeCallback) loadDataContextDispose,
                CLII((struct att[])
                {
                    { STRI("header"), 0, &(handlerContext) { offsetof(loadDataContext, bits), LOADDATACONTEXT_BIT_POS_HEADER }, (readHandlerCallback) boolHandler },
                    { STRI("logarithm"), 0, &(handlerContext) { offsetof(loadDataContext, bits), LOADDATACONTEXT_BIT_POS_LOGARITHM }, (readHandlerCallback) boolHandler },
                    { STRI("no.ranks"), 0, &(handlerContext) { offsetof(loadDataContext, bits), LOADDATACONTEXT_BIT_POS_NORANKS }, (readHandlerCallback) boolHandler },
                    { STRI("path.chr"), offsetof(loadDataContext, pathchr), NULL, (readHandlerCallback) strHandler },
                    { STRI("path.row"), offsetof(loadDataContext, pathrow), NULL, (readHandlerCallback) strHandler },
                    { STRI("path.test"), offsetof(loadDataContext, pathtest), NULL, (readHandlerCallback) strHandler },
                    { STRI("path.val"), offsetof(loadDataContext, pathval), NULL, (readHandlerCallback) strHandler },
                }),
                { dscsch.node, dscsch.cnt }
            },
            {
                .name = STRI("Genotypes"),
                .sz = sizeof(genotypesContext),
                .prologue = (prologueCallback) genotypesPrologue,
                .epilogue = (epilogueCallback) genotypesEpilogue,
                .dispose = (disposeCallback) genotypesContextDispose,
                CLII((struct att[])
                {
                    { STRI("option0"), 0, &(handlerContext) { offsetof(genotypesContext, bits), GENOTYPESCONTEXT_BIT_POS_OPTION0 }, (readHandlerCallback) boolHandler },
                    { STRI("option1"), 0, &(handlerContext) { offsetof(genotypesContext, bits), GENOTYPESCONTEXT_BIT_POS_OPTION1 }, (readHandlerCallback) boolHandler },
                    { STRI("option2"), 0, &(handlerContext) { offsetof(genotypesContext, bits), GENOTYPESCONTEXT_BIT_POS_OPTION2 }, (readHandlerCallback) boolHandler },
                    { STRI("option3"), 0, &(handlerContext) { offsetof(genotypesContext, bits), GENOTYPESCONTEXT_BIT_POS_OPTION3 }, (readHandlerCallback) boolHandler },
                    { STRI("path.bed"), offsetof(genotypesContext, path_bed), NULL, (readHandlerCallback) strHandler },
                    { STRI("path.bim"), offsetof(genotypesContext, path_bim), NULL, (readHandlerCallback) strHandler },
                    { STRI("path.fam"), offsetof(genotypesContext, path_fam), NULL, (readHandlerCallback) strHandler }
                }),
                { dscsch.node, dscsch.cnt }
            },
            {
                .name = STRI("MySQL.Fetch"),
                .sz = sizeof(mysqlFetchContext),
                .prologue = (prologueCallback) mysqlFetchPrologue,
                .epilogue = (epilogueCallback) mysqlFetchEpilogue,
                .dispose = (disposeCallback) mysqlFetchContextDispose,
                CLII((struct att[])
                {
                    { STRI("host"), offsetof(mysqlFetchContext, host), NULL, (readHandlerCallback) strHandler },
                    { STRI("login"), offsetof(mysqlFetchContext, login), NULL, (readHandlerCallback) strHandler },
                    { STRI("no.ranks"), 0, &(handlerContext) { offsetof(mysqlFetchContext, bits), MYSQLFETCHCONTEXT_BIT_POS_NORANKS }, (readHandlerCallback) boolHandler },
                    { STRI("password"), offsetof(mysqlFetchContext, password), NULL, (readHandlerCallback) strHandler },
                    { STRI("port"), offsetof(mysqlFetchContext, port), NULL, (readHandlerCallback) uint32Handler },
                    { STRI("schema"), offsetof(mysqlFetchContext, schema), NULL, (readHandlerCallback) strHandler }
                }),
                { dscsch.node, dscsch.cnt }
            }
        })
    };

    */
    
    //FILE *f = stderr; // fopen("f:\\test\\h", "w");
    //fputs("\xef\xbb\xbf", f);
    //for (size_t i = 0; i < (size_t) argc; i++) fprintf(stderr, "%s\n", argv[i]);
    //fclose(f);
    /*
    struct persistent_array arr = { 0 };
    persistent_array_init(&arr, 126, 8);
    persistent_array_test(&arr, 512, 8);
    *(size_t *) persistent_array_fetch(&arr, 127, 8) = 10;
    *(size_t *) persistent_array_fetch(&arr, 510, 8) = 10;
    */

    //lm_test();

    bool succ = 1;
    struct ttl_style ttl_style = {
        .time_stamp = ENVI_FG(FG_BR_BLACK),
        .header = { ENVI_FG(FG_GREEN), ENVI_FG(FG_RED), ENVI_FG(FG_YELLOW), ENVI_FG(FG_MAGENTA), ENVI_FG(FG_CYAN), ENVI_FG(FG_BR_BLACK) },
        .code_metric = ENVI_FG(FG_BR_BLACK)
    };

    struct style style = {
        .type_int = ENVI_FG(FG_BR_CYAN),
        .type_char = ENVI_FG(FG_BR_MAGENTA),
        .type_path = ENVI_FG(FG_BR_CYAN),
        .type_str = ENVI_FG(FG_BR_MAGENTA),
        .type_flt = ENVI_FG(FG_BR_CYAN),
        .type_time_diff = ENVI_FG(FG_BR_YELLOW),
        .type_code_metric = ENVI_FG(FG_BR_YELLOW),
        .type_utf = ENVI_FG(FG_BR_MAGENTA)
        // ENV_INIT_COL_EXT(UTF8_LSQUO, FG_BR_MAGENTA, UTF8_RSQUO)
    };

    struct log fallback; // Slow fallback log for all operations. Destination device: stderr; 
    if (log_init(&fallback, NULL, 0, LOG_WRITE_LOCK, &ttl_style, &style, NULL))
    {
        //struct log log1;
        //log_init(&log1, NULL, 1 + 0 * BLOCK_WRITE, 0, &ttl_style, &style, &log);
        //log_message_fmt(&log1, CODE_METRIC, MESSAGE_NOTE, "%@@$%$%~T.\n", (const void *[]) { "012345678901%~~-sAA%!-s1;%1;C%$F", &(struct env) ENV_INIT_COL(FG_BR_CYAN), "X", "BB", "%$E", "D" }, "G%%%~~#*%~#*%''#x394%~#921", &(struct env) ENV_INIT_COL(FG_BR_CYAN), 0x393, 920, 0ull, UINT64_C(12041241241));
        //log_message_fmt(&log1, CODE_METRIC, MESSAGE_NOTE, "%@@$%$", (const void *[]) { "AA%!-s1;%1;C%$F", "B", "%$E", "D" }, "G%%%~#*%~#*%~c.\n", 0x393, 920, 'a');
        //log_message_fmt(&log1, CODE_METRIC, MESSAGE_NOTE, "|%$%$", "%c4%!-sX%3|", 'W', "YYY", "\n");
        //exit(0);
        //SetLastError(99999);
        //wapi_assert(&log1, CODE_METRIC, 0);
        //wapi_assert(&log1, CODE_METRIC, 0);

        //struct persistent_array a;
        //persistent_array_init(&a, 1, 100, 0);
        //persistent_array_test(&a, 3, 100, 0);
        //void *z = persistent_array_fetch(&a, 0, 100);
        //z = persistent_array_fetch(&a, 4, 100);

        //struct thread_pool *pool = thread_pool_create(4, 0, 1, &log);
        //thread_pool_schedule(pool);
        
        size_t pos_cnt;
        char **pos_arr;
        struct main_args main_args = { 0 };
        if (argv_parse(argv_par_selector, &argv_par_sch, &main_args, argv, argc, &pos_arr, &pos_cnt, &fallback))
        {
            main_args = main_args_override(main_args, main_args_default());
            struct log log;
            if (log_init(&log, main_args.log_path, BLOCK_WRITE, LOG_WRITE_LOCK, &ttl_style, &style, &fallback))
            {
                wapi_assert(&log, CODE_METRIC, 0);
                /*
                if (uint8_bit_test(main_args.bits, MAIN_ARGS_BIT_POS_FANCY))
                {
                    if (uint8_bit_test(main_args.bits, MAIN_ARGS_BIT_POS_FANCY_USER))
                        log_message_fmt(&log, CODE_METRIC, MESSAGE_INFO, "Fancy: ON.\n");
                    else
                        log_message_fmt(&log, CODE_METRIC, MESSAGE_INFO, "Fancy: set to DEFAULT.\n");
                }
                else
                {
                    if (uint8_bit_test(main_args.bits, MAIN_ARGS_BIT_POS_FANCY_USER))
                        log_message_fmt(&log, CODE_METRIC, MESSAGE_INFO, "Fancy: OFF.\n");
                    else
                        log_message_fmt(&log, CODE_METRIC, MESSAGE_INFO, "Fancy: DEFAULT.\n");
                }
                */

                if (uint8_bit_test(main_args.bits, MAIN_ARGS_BIT_POS_HELP))
                {
                    // Help mode
                    log_message_fmt(&log, CODE_METRIC, MESSAGE_INFO, "Help mode triggered!\n");
                }
                else if (uint8_bit_test(main_args.bits, MAIN_ARGS_BIT_POS_TEST))
                {
                    succ = test_main(main_args.thread_cnt, &log);
                }
                else if (uint8_bit_test(main_args.bits, MAIN_ARGS_BIT_POS_CAT))
                {
                    if (pos_cnt >= 7)
                    {
                        size_t rpl = (size_t) strtoull(pos_arr[5], NULL, 10);
                        uint64_t seed = (uint64_t) strtoull(pos_arr[6], NULL, 10);
                        succ = categorical_run_adj(pos_arr[0], pos_arr[1], pos_arr[2], pos_arr[3], pos_arr[4], rpl, seed, &log);
                    }
                }
                else if (uint8_bit_test(main_args.bits, MAIN_ARGS_BIT_POS_LDE))
                {
                    if (pos_cnt >= 2) lde_run(pos_arr[0], pos_arr[1], &fallback);
                }
                else if (uint8_bit_test(main_args.bits, MAIN_ARGS_BIT_POS_CHISQ))
                {
                    if (pos_cnt >= 4)
                    {
                        succ = categorical_run_chisq(pos_arr[0], pos_arr[1], pos_arr[2], pos_arr[3], &log);
                    }
                }
                else if (uint8_bit_test(main_args.bits, MAIN_ARGS_BIT_POS_LM))
                {
                    if (pos_cnt >= 5)
                    {
                        succ = lm_expr_test(pos_arr[0], pos_arr[1], pos_arr[2], pos_arr[3], pos_arr[4], &log);
                    }
                }
                else
                {
                    if (!pos_cnt) log_message_fmt(&log, CODE_METRIC, MESSAGE_NOTE, "No input data specified.\n");
                    else
                    {

                    }
                }
                log_close(&log);
            }
            free(pos_arr);
        }
        log_close(&fallback);
    }
    
    /*   
    if (bitTest(args.bits, CMDARGS_BIT_POS_HELP))
    {
        logMsg(&loginfo, strings[STR_FR_IH], strings[STR_FN]);

        // Help message should be here
    }
    else
    {
        if (!inputcnt) logMsg(&loginfo, strings[STR_FR_ND], strings[STR_FN]);

        for (size_t i = 0; i < inputcnt; i++) // Processing input files
        {
            programObject *obj = programObjectFromXML(&xmlsch, input[i], &loginfo);

            if (obj)
            {
                if (!programObjectExecute(obj, &args.in)) logMsg(&loginfo, strings[STR_FR_WE], strings[STR_FN], input[i]);
                programObjectDispose(obj);
            }
            else logMsg(&loginfo, strings[STR_FR_WC], strings[STR_FN], input[i]);
        }
    }
    */
   
    return succ ? EXIT_SUCCESS : EXIT_FAILURE;
}

#if TEST(IFN_WIN)

int main(int argc, char **argv)
{
    return Main(argc, argv);
}

#else

#   include <windows.h>

// Determining total buffer length (with null-terminators) and performing error checking
static bool wargv_cnt_impl(size_t argc, wchar_t **wargv, size_t *p_cnt)
{
    size_t cnt = argc; // Size of the null-terminators
    for (size_t i = 0; i < argc; i++)
    {
        uint32_t val;
        uint8_t context = 0;
        for (wchar_t *word = wargv[i]; *word; word++)
        {
            if (!utf16_decode((uint16_t) *word, &val, NULL, NULL, &context, 0)) return 0;
            if (context) continue;
            size_t car;
            cnt = size_add(&car, cnt, utf8_len(val));
            if (car) return 0;
        }
        if (context) return 0;
    }
    *p_cnt = cnt;
    return 1;
}

// Performing translation from UTF-16LE to UTF-8
static bool wargv_to_argv_impl(size_t argc, wchar_t **wargv, char *base, char **argv)
{
    for (size_t i = 0; i < argc; i++)
    {
        argv[i] = base;
        uint32_t val;
        uint8_t context = 0;
        for (wchar_t *word = wargv[i]; *word; word++)
        {
            _Static_assert(sizeof(*word) == sizeof(uint16_t), "");
            if (!utf16_decode((uint16_t) *word, &val, NULL, NULL, &context, 0)) return 0;
            if (context) continue;
            uint8_t len;
            utf8_encode(val, (uint8_t *) base, &len);
            base += len;
        }
        if (context) return 0;
        *(base++) = '\0';
    }
    return 1;
}

// Translating UTF-16LE command-line parameters to UTF-8
static bool argv_from_wargv(char ***p_argv, size_t argc, wchar_t **wargv)
{
    char **argv;
    if (array_init(&argv, NULL, argc, sizeof(*argv), 0, ARRAY_STRICT).status)
    {
        char *base;
        size_t cnt;
        if (wargv_cnt_impl(argc, wargv, &cnt) && array_init(&base, NULL, cnt, sizeof(*base), 0, ARRAY_STRICT).status)
        {
            if (wargv_to_argv_impl(argc, wargv, base, argv)) // Should never fail: all checks are done in the previous 'if' clause
            {
                *p_argv = argv;
                return 1;
            }
            free(base);
        }
        free(argv);
    }    
    return 0;
}

static void argv_dispose(size_t argc, char **argv)
{
    if (argc) free(argv[0]);
    free(argv);
}

static int Wmain(int argc, wchar_t **wargv)
{
    // Memory leaks will be reported at the program exit
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

    // Trying to make console output UTF-8 friendly
    SetConsoleOutputCP(CP_UTF8);
    
    // Trying to enable handling for VT sequences
    HANDLE ho = GetStdHandle(STD_ERROR_HANDLE);
    if (ho && ho != INVALID_HANDLE_VALUE)
    {
        DWORD mode = 0;
        if (GetConsoleMode(ho, &mode)) SetConsoleMode(ho, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_WRAP_AT_EOL_OUTPUT);
    }

    // Performing UTF-16LE to UTF-8 translation and executing the main routine
    char **argv;
    if (!argv_from_wargv(&argv, argc, wargv)) return EXIT_FAILURE;
    int main_res = Main(argc, argv);
    argv_dispose(argc, argv);
    return main_res;
}

#   if TEST(IF_MINGW)
#   include <shellapi.h>

int main()
{
    int argc;
    wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!wargv) return EXIT_FAILURE;
    int main_res = Wmain(argc, wargv);
    LocalFree(wargv);
    return main_res;
}

#   else

int wmain(int argc, wchar_t **wargv)
{
    return Wmain(argc, wargv);
}

#   endif

#endif
