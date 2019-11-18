#pragma once

#include "common.h"
#include "intdom.h"
#include "strproc.h"
#include "log.h"

#include <stdbool.h>

#define XML_DECL "<?xml"

typedef bool (*prologue_callback)(void *, void **, void *);
typedef bool (*epilogue_callback)(void *, void *, void *);
typedef void (*disposer_callback)(void *);
typedef bool (*text_handler_callback)(void *, void *, char *, size_t);

struct xml_object;

void xml_object_dispose(struct xml_object *);
bool xml_object_execute(struct xml_object *, void *);

struct xml_val {
    void *ptr, *context;
    read_callback handler;
};

struct xml_att {
    void *ptr, *context;
    read_callback handler;
};

/*struct xml_node {
    //struct strl name;
    size_t sz;
    prologue_callback prologue;
    epilogue_callback epilogue;
    disposer_callback dispose;    
    //struct {
    //    struct xml_node *dsc;
    //    size_t dsc_cnt;
    //};
};*/

struct base_context {
    struct buff *buff;
    //struct str_pool *pool;
    struct text_metric metric; // metric snapshot
    unsigned st; // state
    size_t len; // register for length
    union int_val val;
    void *context;
};

typedef bool (*xml_node_selector_callback)(struct xml_node *, const char *, size_t, void *, void *);
typedef bool (*xml_val_selector_callback)(struct xml_att *, const char *, size_t, void *, void *, size_t *);

enum xml_status_chr {
    XML_UNEXPECTED_EOF = 0,
    XML_UNEXPECTED_CHAR
};

enum xml_status {
    XML_INVALID_UTF = 0,
    XML_INVALID_CHAR,
    XML_INVALID_DECL,
    XML_MISSING_ROOT,
    XML_OUT_OF_RANGE,
    XML_ERROR_COMPILER
};

bool log_message_xml_generic(struct log *restrict, struct code_metric, enum message_type, struct text_metric, ...);
bool log_message_error_xml_chr(struct log *restrict, struct code_metric, struct text_metric, enum xml_status_chr, const uint8_t *, size_t);
bool log_message_error_xml(struct log *restrict, struct code_metric, struct text_metric, enum xml_status);

struct xml_object *xml_compile(const char *, xml_node_selector_callback, xml_val_selector_callback, void *, struct log *);
