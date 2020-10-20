#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Command-line parser which is nearly equal to 'getopt_long' GNU function
//

#include "common.h"
#include "log.h"
#include "cmp.h"

enum par_mode {
    PAR_VALUED = 0,
    PAR_OPTION,
    PAR_VALUED_OPTION
};

struct par {
    void *ptr, *context; // Third argument of the handler
    read_callback handler;
    enum par_mode mode;
};

typedef bool (*par_selector_callback)(struct par *, const char *, size_t, void *, void *, bool);
bool argv_parse(par_selector_callback, void *, void *, char **, size_t, char ***, size_t *, struct log *);

struct tag {
    struct strl name;
    size_t id;
};

struct par_sch {
    size_t off;
    void *context; // Third argument of the handler
    read_callback handler;
    enum par_mode mode;
};

struct argv_par_sch {
    struct {
        struct tag *ltag; // Long tag name handling; array should be sorted by 'name' according to the 'strncmp'
        size_t ltag_cnt;
    };
    struct {
        struct tag *stag; // Short tag name handling; 'name' is UTF-8 byte sequence; array should be sorted by 'ch'
        size_t stag_cnt;
    };
    struct {
        struct par_sch *par_sch;
        size_t par_sch_cnt;
    };
};

bool argv_par_selector(struct par *, const char *, size_t len, void *, void *, bool);
