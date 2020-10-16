#include "np.h"
#include "ll.h"
#include "utf8.h"
#include "memory.h"

#include <string.h>
#include <immintrin.h>

#if TEST(IF_UNIX_APPLE)

#   include <stdlib.h>
#   include <strings.h>
#   include <unistd.h>
#   include <fcntl.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <sys/time.h>

#elif TEST(IF_WIN)

#   include <windows.h>
#   include <wchar.h>
#   include <io.h>
#   include <sys/stat.h>

#   define WSTR_CMP(WSTR, LEN, CMP) \
        ((LEN) >= (countof(CMP) - 1) && \
        !wcsncmp((WSTR), (CMP), (countof(CMP) - 1)) && \
        (WSTR += (countof(CMP) - 1), LEN -= (countof(CMP) - 1), 1))

static bool utf8_str_to_wstr(const char *restrict str, wchar_t **p_wstr, size_t *p_wlen)
{
    uint32_t val;
    uint8_t ulen = 0, context = 0;
    size_t len = 0, wcnt = 1; // Space for the null-terminator
    for (char ch = str[len]; ch; ch = str[++len])
    {
        if (!utf8_decode(ch, &val, NULL, &ulen, &context)) return 0;
        if (context) continue;
        if (!test_add(&wcnt, utf16_len(val))) return 0;
    }
    if (context) return 0;
    wchar_t *wstr;
    if (!array_init(&wstr, NULL, wcnt, sizeof(*wstr), 0, ARRAY_STRICT).status) return 0;
    for (size_t i = 0, j = 0; i < len; i++)
    {
        utf8_decode(str[i], &val, NULL, NULL, &context); // No checks are required
        if (context) continue;
        uint8_t tmp;
        _Static_assert(sizeof(*wstr) == sizeof(uint16_t), "");
        utf16_encode(val, (uint16_t *) wstr + j, &tmp, 0);
        j += tmp;
    }
    wstr[wcnt - 1] = L'\0';
    *p_wstr = wstr;
    if (p_wlen) *p_wlen = wcnt - 1;
    return 1;
}

#endif

#define IO_POSIX(X) IF_UNIX_APPLE(X) IF_WIN(_ ## X)
#define IO_UNLOCKED(X) IF_UNIX(X ## _unlocked) IF_APPLE(X) IF_WIN(_ ## X ## _nolock) 
#define IO_FSTAT(X) IF_UNIX_APPLE(X) IF_WIN(_ ## X ## 64) 

void *Realloc(void *ptr, size_t sz)
{
    if (sz) return realloc(ptr, sz);
    free(ptr);
    return NULL;
}

IF_UNIX_APPLE(void *Aligned_malloc(size_t al, size_t sz)
{
    void *res;
    int code = posix_memalign(&res, al, sz);
    if (code) errno = code;
    return res;
})

IF_WIN(void *Aligned_malloc(size_t al, size_t sz)
{
    return _aligned_malloc(sz, al);
})

IF_UNIX_APPLE(void *Aligned_realloc(void *ptr, size_t al, size_t sz)
{
    (void) ptr;
    (void) al;
    if (sz) errno = ENOSYS; // Function not implemented
    else Aligned_free(ptr);
    return NULL;
})

IF_WIN(void *Aligned_realloc(void *ptr, size_t al, size_t sz)
{
    if (sz) return _aligned_realloc(ptr, sz, al);
    Aligned_free(ptr);
    return NULL;
})

IF_UNIX_APPLE(void *Aligned_calloc(size_t al, size_t cnt, size_t sz)
{
    if (!test_mul(&sz, cnt)) return errno = ENOMEM, NULL;
    void *res = Aligned_malloc(al, sz);
    if (!res) return NULL;
    memset(res, 0, sz);
    return res;
})

IF_WIN(void *Aligned_calloc(size_t al, size_t cnt, size_t sz)
{
    return sz ? _aligned_recalloc(NULL, cnt, sz, al) : NULL;
})

IF_UNIX_APPLE(void Aligned_free(void *ptr)
{
    free(ptr);
})

IF_WIN(void Aligned_free(void *ptr)
{
    _aligned_free(ptr);
})

IF_UNIX_APPLE(FILE *Fopen(const char *path, const char *mode)
{
    return fopen(path, mode);
})

IF_WIN(FILE *Fopen(const char *path, const char *mode)
{
    FILE *f = NULL;
    wchar_t *wpath = NULL, *wmode = NULL;
    if (utf8_str_to_wstr(path, &wpath, NULL) && utf8_str_to_wstr(mode, &wmode, NULL)) f = _wfopen(wpath, wmode);
    else if (!errno) errno = EINVAL;
    free(wpath);
    free(wmode);
    return f;
})

FILE *Fdup(FILE *f, const char *mode)
{
    int fd = IO_POSIX(dup)(IO_POSIX(fileno)(f));
    if (fd == -1) return NULL;
    return IO_POSIX(fdopen)(fd, mode);
}

size_t Fwrite_unlocked(const void *ptr, size_t sz, size_t cnt, FILE *file)
{
    return IO_UNLOCKED(fwrite)(ptr, sz, cnt, file);
}

int Fflush_unlocked(FILE *file)
{
    return IO_UNLOCKED(fflush)(file);
}

IF_UNIX_APPLE(int Fseeki64(FILE *file, int64_t offset, int origin)
{
    return fseeko(file, (off_t) offset, origin);
})

IF_WIN(int Fseeki64(FILE *file, int64_t offset, int origin)
{
    return _fseeki64(file, offset, origin);
})

IF_UNIX_APPLE(int64_t Ftelli64(FILE *file)
{
    return (int64_t) ftello(file);
})

IF_WIN(int64_t Ftelli64(FILE *file)
{
    return _ftelli64(file);
})

// The implementation below is inspired by:
// https://github.com/k-takata/ptycheck
// https://github.com/msys2/MINGW-packages/blob/master/mingw-w64-gcc/0140-gcc-8.2.0-diagnostic-color.patch
bool Fisatty(FILE *f)
{
    int fd = IO_POSIX(fileno)(f);
    if (IO_POSIX(isatty)(fd)) return 1;
#if TEST(IF_WIN)
    DWORD mode;
    HANDLE ho = (HANDLE) _get_osfhandle(fd);
    if (!ho || ho == INVALID_HANDLE_VALUE) return 0;
    if (GetConsoleMode(ho, &mode)) return 1;
    if (GetFileType(ho) != FILE_TYPE_PIPE) return 0;
    wchar_t wbuff[MAX_PATH], *wstr = wbuff; // 'MAX_PATH' already provides space for the null-terminator
    DWORD len = GetFinalPathNameByHandleW(ho, (LPWSTR) wstr, countof(wbuff), VOLUME_NAME_NONE | FILE_NAME_OPENED), tmp;
    // Compare to a sample string, e. g. L"\\msys-dd50a72ab4668b33-pty1-to-master"
    if (!len || len > countof(wbuff) || (!WSTR_CMP(wstr, len, L"\\msys-") && !WSTR_CMP(wstr, len, L"\\cygwin-"))) return 0; // Skip prefix: L"\\cygwin-" or L"\\msys-"
    for (tmp = 0; len && iswxdigit(*wstr); tmp++, wstr++, len--); // Skip 16 hexadecimal digits
    if (tmp != 16 || !WSTR_CMP(wstr, len, L"-pty")) return 0; // Skip L"-pty"
    for (tmp = 0; len && iswdigit(*wstr); tmp++, wstr++, len--); // Skip at least one digit
    if (!tmp || (!WSTR_CMP(wstr, len, L"-from-master") && !WSTR_CMP(wstr, len, L"-to-master"))) return 0; // Skip suffix: L"-from-master" or L"-to-master"
    return !len;
#endif
}

int64_t Fsize(FILE *f)
{
    struct IO_FSTAT(stat) st;
    return IO_FSTAT(fstat)(IO_POSIX(fileno)(f), &st) ? 0 : (int64_t) st.st_size;
}

int Fclose(FILE *f)
{
    return f ? fclose(f) : 0;
}

IF_UNIX_APPLE(Errno_t Strerror_s(char *buff, size_t cap, Errno_t code)
{
    return strerror_r(code, buff, cap);
})

IF_WIN(Errno_t Strerror_s(char *buff, size_t cap, Errno_t code)
{
    return strerror_s(buff, cap, code);
})

IF_UNIX_APPLE(Errno_t Localtime_s(struct tm *tm, const time_t *t)
{
    return localtime_r(t, tm) ? 0 : errno;
})

IF_WIN(Errno_t Localtime_s(struct tm *tm, const time_t *t)
{
    return localtime_s(tm, t);
})

IF_UNIX_APPLE(int Stricmp(const char *a, const char *b)
{
    return strcasecmp(a, b);
})

IF_WIN(int Stricmp(const char *a, const char *b)
{
    return _stricmp(a, b);
})

IF_UNIX_APPLE(int Strnicmp(const char *a, const char *b, size_t len)
{
    return strncasecmp(a, b, len);
})

IF_WIN(int Strnicmp(const char *a, const char *b, size_t len)
{
    return _strnicmp(a, b, len);
})

IF_UNIX_APPLE(size_t get_processor_count()
{
    return (size_t) sysconf(_SC_NPROCESSORS_CONF);
})

IF_WIN(size_t get_processor_count()
{
    SYSTEM_INFO inf;
    GetSystemInfo(&inf);
    return (size_t) inf.dwNumberOfProcessors;
})

IF_UNIX_APPLE(size_t get_page_size()
{
    return (size_t) sysconf(_SC_PAGESIZE);
})

IF_WIN(size_t get_page_size()
{
    SYSTEM_INFO inf;
    GetSystemInfo(&inf);
    return (size_t) inf.dwPageSize;
})

IF_UNIX_APPLE(size_t get_process_id()
{
    return (size_t) getpid();
})

IF_WIN(size_t get_process_id()
{
    return (size_t) GetCurrentProcessId();
})

IF_UNIX_APPLE(uint64_t get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t) tv.tv_sec * 1000000 + (uint64_t) tv.tv_usec;
})

IF_WIN(uint64_t get_time()
{
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    uint64_t t = (uint64_t) ft.dwHighDateTime << (sizeof(uint32_t) * CHAR_BIT) | (uint64_t) ft.dwLowDateTime;
    return t / 10 + !!(t % 10);
})

size_t Strnlen(const char *str, size_t len)
{
    char *end = memchr(str, '\0', len);
    return end ? (size_t) (end - str) : len;
}

// Similar to BSD 'strchrnul', but returns index instead of pointer. Index is set to length if character not found
// Bonus: 'ch' may contain up to four characters simultaneously 
size_t Strchrnull(const char *str, int ch)
{
    int rest = (uintptr_t) str % alignof(__m128i);
    __m128i msk = _mm_cvtsi32_si128(ch);
    size_t off = 0;
    if (rest)
    {
        // Loading residual part from the lower 16-byte boundary of the string
        unsigned ind = uint_bsf((unsigned) _mm_cvtsi128_si32(_mm_cmpestrm(msk, sizeof(__m128i), _mm_load_si128((__m128i *) (str - rest)), sizeof(__m128i), _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_POSITIVE_POLARITY | _SIDD_BIT_MASK)) >> rest);
        if (ind != umax(ind)) return ind;
        off = sizeof(__m128i) - rest;
    }
    for (;;)
    {
        // Loading aligned part of the string
        int ind = _mm_cmpestri(msk, sizeof(__m128i), _mm_load_si128((__m128i *) (str + off)), sizeof(__m128i), _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_POSITIVE_POLARITY | _SIDD_LEAST_SIGNIFICANT);
        if (ind < sizeof(__m128i)) return off + ind;
        off += sizeof(__m128i);
    }
}

size_t Strlncpy(char *dst, char *src, size_t cnt)
{
    size_t len = Strnlen(src, cnt);
    memcpy(dst, src, len + (len < cnt));
    return len;
}

void *Memrchr(const void *Str, int ch, size_t cnt)
{
    const __m128i msk = _mm_set1_epi8((char) ch);
    const char *str = (const char *) Str;
    const size_t off = ((uintptr_t) Str + cnt) % alignof(__m128i), n = MIN(off, cnt);
    for (size_t i = 0; i < n; i++) if (str[--cnt] == ch) return (void *) (str + cnt);
    while (cnt >= sizeof(__m128i))
    {
        cnt -= sizeof(__m128i);
        __m128i a = _mm_cmpeq_epi8(_mm_load_si128((const __m128i *) (str + cnt)), msk);
        if (!_mm_testz_si128(a, a)) return (void *) (str + cnt + m128i_b8sr(a));
    }
    while (cnt) if (str[--cnt] == ch) return (void *) (str + cnt);
    return NULL;
}

void *Memrchr2(const void *Str, int ch0, int ch1, size_t cnt)
{
    const __m128i msk0 = _mm_set1_epi8((char) ch0), msk1 = _mm_set1_epi8((char) ch1);
    const char *str = (const char *) Str;
    const size_t off = ((uintptr_t) Str + cnt) % alignof(__m128i), n = MIN(off, cnt);
    for (size_t i = 0; i < n; i++) if (str[--cnt] == ch0 || str[cnt] == ch1) return ( void *) (str + cnt);
    while (cnt >= sizeof(__m128i))
    {
        cnt -= sizeof(__m128i);
        __m128i tmp = _mm_load_si128((const __m128i *) (str + cnt));
        __m128i a = _mm_or_si128(_mm_cmpeq_epi8(tmp, msk0), _mm_cmpeq_epi8(tmp, msk1));
        if (!_mm_testz_si128(a, a)) return ( void *) (str + cnt + m128i_b8sr(a));
    }
    while (cnt) if (str[--cnt] == ch0 || str[cnt] == ch1) return ( void *) (str + cnt);
    return NULL;
}
