#include "lm.h"
#include "sort.h"
#include "strproc.h"

#include <string.h>

size_t str_off_x33_hash(const void *Off, void *Str)
{
    return str_x33_hash((char *) Str + *(size_t *) Off, NULL);
}

bool str_off_eq(const void *A, const void *B, void *Str)
{
    return strcmp((char *) Str + *(size_t *) A, (char *) Str + *(size_t *) B);
}

void lm_test()
{
    char str[] = "ABC\0EFD\0ABCD";
    size_t off[] = { 0, 4, 8 };
    int val[] = { 1, 2, 3 }, tmp;
    struct hash_table tbl = { .cnt = 0 };
    unsigned res;
    res = hash_table_insert(&tbl, off + 0, sizeof(*off), val + 0, sizeof(*val), str_off_x33_hash, str_off_eq, str);
    res = hash_table_insert(&tbl, off + 0, sizeof(*off), val + 0, sizeof(*val), str_off_x33_hash, str_off_eq, str);
    res = hash_table_insert(&tbl, off + 1, sizeof(*off), val + 1, sizeof(*val), str_off_x33_hash, str_off_eq, str);
    //hash_table_remove(&tbl, )
    res = hash_table_insert(&tbl, off + 1, sizeof(*off), val + 1, sizeof(*val), str_off_x33_hash, str_off_eq, str);
    res = hash_table_search(&tbl, off + 1, sizeof(*off), &tmp, sizeof(tmp), str_off_x33_hash, str_off_eq, str);
    res = hash_table_search(&tbl, off + 2, sizeof(*off), &tmp, sizeof(tmp), str_off_x33_hash, str_off_eq, str);
}

struct covariate {
    size_t *deg;
    uint8_t *bits;
};

void lm_parse(const char *eq)
{

}