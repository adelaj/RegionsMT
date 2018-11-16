#pragma once

#include "common.h"
#include "strproc.h"
#include "log.h"

#include <stdbool.h>

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

typedef bool (*xml_node_selector_callback)(struct xml_node *, const char *, size_t, void *, void *);
typedef bool (*xml_val_selector_callback)(struct xml_att *, const char *, size_t, void *, void *, size_t *);

struct xml_object *xml_compile(const char *, xml_node_selector_callback, xml_val_selector_callback, void *, struct log *);
