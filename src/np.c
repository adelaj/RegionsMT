#include "np.h"
#include "ll.h"
#include "utf8.h"
#include "memory.h"

#include <string.h>
#include <immintrin.h>

#if defined _WIN32 && (defined _MSC_BUILD || defined __MINGW32__)

#   include <windows.h>
#   include <wchar.h>
#   include <io.h>
#   include <sys/stat.h>

void *Aligned_malloc(size_t al, size_t sz)
{
    return _aligned_malloc(sz, al);
}

void *Aligned_realloc(void *ptr, size_t al, size_t sz)
{
    if (sz) return _aligned_realloc(ptr, sz, al);
    Aligned_free(ptr);
    return NULL;
}

void *Aligned_calloc(size_t al, size_t cnt, size_t sz)
{
    return sz ? _aligned_recalloc(NULL, cnt, sz, al) : NULL;
}

void Aligned_free(void *ptr)
{
    _aligned_free(ptr);
}

static bool utf8_str_to_wstr(const char *restrict str, wchar_t **p_wstr, size_t *p_wlen)
{
    uint32_t val;
    uint8_t ulen = 0, context = 0;
    size_t len = 0, wcnt = 1; // Space for the null-terminator
    for (char ch = str[len]; ch; ch = str[++len])
    {
        if (!utf8_decode(ch, &val, NULL, &ulen, &context)) return 0;
        if (context) continue;
        size_t car;
        wcnt = size_add(&car, wcnt, utf16_len(val));
        if (car) return 0;
    }
    if (context) return 0;
    wchar_t *wstr;
    if (!array_init(&wstr, NULL, wcnt, sizeof(*wstr), 0, ARRAY_STRICT).status) return 0;
    for (size_t i = 0, j = 0; i < len; i++)
    {
        utf8_decode(str[i], &val, NULL, NULL, &context); // No checks are required
        if (context) continue;
        uint8_t tmp;
        utf16_encode(val, (uint16_t *) wstr + j, &tmp, 0);
        j += tmp;
    }
    wstr[wcnt - 1] = L'\0';
    *p_wstr = wstr;
    if (p_wlen) *p_wlen = wcnt - 1;
    return 1;
}

FILE *Fopen(const char *path, const char *mode)
{
    FILE *f = NULL;
    wchar_t *wpath = NULL, *wmode = NULL;
    if (utf8_str_to_wstr(path, &wpath, NULL) && utf8_str_to_wstr(mode, &wmode, NULL)) f = _wfopen(wpath, wmode);
    else if (!errno) errno = EINVAL;
    free(wpath);
    free(wmode);
    return f;
}

FILE *Fdup(FILE *f, const char *mode)
{
    int fd = _dup(_fileno(f));
    if (fd == -1) return NULL;
    return _fdopen(fd, mode);
}

size_t Fwrite_unlocked(const void *ptr, size_t sz, size_t cnt, FILE *file)
{
    return _fwrite_nolock(ptr, sz, cnt, file);
}

int Fflush_unlocked(FILE *file)
{
    return _fflush_nolock(file);
}

int Fseeki64(FILE *file, int64_t offset, int origin)
{
    return _fseeki64(file, offset, origin);
}

int64_t Ftelli64(FILE *file)
{
    return _ftelli64(file);
}

#define WSTR_CMP(WSTR, LEN, CMP) \
    ((LEN) >= (countof(CMP) - 1) && \
    !wcsncmp((WSTR), (CMP), (countof(CMP) - 1)) && \
    (WSTR += (countof(CMP) - 1), LEN -= (countof(CMP) - 1), 1))

// The implementation below is inspired by:
// https://github.com/k-takata/ptycheck
// https://github.com/msys2/MINGW-packages/blob/master/mingw-w64-gcc/0140-gcc-8.2.0-diagnostic-color.patch
bool Fisatty(FILE *f)
{
    int fd = _fileno(f);
    if (_isatty(fd)) return 1;
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
}

int64_t Fsize(FILE *f)
{
<<<<<<< HEAD
    struct _stat64 st;
    if (!_fstat64(_fileno(f), &st)) return (int64_t) st.st_size;
    else return 0;
=======
    LARGE_INTEGER sz;
    HANDLE ho = (HANDLE) _get_osfhandle(_fileno(f));
    return ho && ho != INVALID_HANDLE_VALUE && GetFileSizeEx(ho, &sz) ? (int64_t) sz.QuadPart : 0;
>>>>>>> 2fa3c4951c93e154eb4ce77eb424ca0c0561eb8c
}

Errno_t Strerror_s(char *buff, size_t cap, Errno_t code)
{
    return strerror_s(buff, cap, code);
}

Errno_t Localtime_s(struct tm *tm, const time_t *t)
{
    return localtime_s(tm, t);
}

int Stricmp(const char *a, const char *b)
{
    return _stricmp(a, b);
}

int Strnicmp(const char *a, const char *b, size_t len)
{
    return _strnicmp(a, b, len);
}

size_t get_processor_count()
{
    SYSTEM_INFO inf;
    GetSystemInfo(&inf);
    return (size_t) inf.dwNumberOfProcessors;
}

size_t get_page_size()
{
    SYSTEM_INFO inf;
    GetSystemInfo(&inf);
    return (size_t) inf.dwPageSize;
}

size_t get_process_id()
{
    return (size_t) GetCurrentProcessId();
}

uint64_t get_time()
{
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    uint64_t t = (uint64_t) ft.dwHighDateTime << (sizeof(uint32_t) * CHAR_BIT) | (uint64_t) ft.dwLowDateTime;
    return t / 10 + !!(t % 10);
}

#elif (defined __GNUC__ || defined __clang__) && (defined __unix__ || defined __APPLE__)

#   include <stdlib.h>
#   include <strings.h>
#   include <unistd.h>
#   include <fcntl.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <sys/time.h>

void *Aligned_malloc(size_t al, size_t sz)
{
    void *res;
    int code = posix_memalign(&res, al, sz);
    if (code) errno = code;
    return res;
}

void *Aligned_realloc(void *ptr, size_t al, size_t sz)
{
    (void) ptr;
    (void) al;
    if (sz) errno = ENOSYS; // Function not implemented
    else Aligned_free(ptr);
    return NULL;
}

void *Aligned_calloc(size_t al, size_t cnt, size_t sz)
{
    size_t hi, tot = size_mul(&hi, cnt, sz);
    if (hi)
    {
        errno = ENOMEM;
        return NULL;
    }
    void *res = Aligned_malloc(al, tot);
    if (!res) return NULL;
    memset(res, 0, tot);
    return res;
}

void Aligned_free(void *ptr)
{
    free(ptr);
}

FILE *Fopen(const char *path, const char *mode)
{
    return fopen(path, mode);
}

FILE *Fdup(FILE *f, const char *mode)
{
    int fd = dup(fileno(f));
    if (fd == -1) return NULL;
    return fdopen(fd, mode);
}

#   ifndef __APPLE__

size_t Fwrite_unlocked(const void *ptr, size_t sz, size_t cnt, FILE *file)
{
    return fwrite_unlocked(ptr, sz, cnt, file);
}

int Fflush_unlocked(FILE *file)
{
    return fflush_unlocked(file);
}

#   else

size_t Fwrite_unlocked(const void *ptr, size_t sz, size_t cnt, FILE *file)
{
    return fwrite(ptr, sz, cnt, file);
}

int Fflush_unlocked(FILE *file)
{
    return fflush(file);
}

#   endif

int Fseeki64(FILE *file, int64_t offset, int origin)
{
    return fseeko(file, (off_t) offset, origin);
}

int64_t Ftelli64(FILE *file)
{
    return (int64_t) ftello(file);
}

bool Fisatty(FILE *f)
{
    return isatty(fileno(f));
}

int64_t Fsize(FILE *f)
{
    struct stat st;
    if (!fstat(fileno(f), &st)) return (int64_t) st.st_size;
    else return 0;
}

Errno_t Strerror_s(char *buff, size_t cap, Errno_t code)
{
    return strerror_r(code, buff, cap);
}

Errno_t Localtime_s(struct tm *tm, const time_t *t)
{
    return localtime_r(t, tm) ? 0 : errno;
}

int Stricmp(const char *a, const char *b)
{
    return strcasecmp(a, b);
}

int Strnicmp(const char *a, const char *b, size_t len)
{
    return strncasecmp(a, b, len);
}

size_t get_processor_count() 
{ 
    return (size_t) sysconf(_SC_NPROCESSORS_CONF);
}

size_t get_page_size() 
{ 
    return (size_t) sysconf(_SC_PAGESIZE);
}

size_t get_process_id() 
{
    return (size_t) getpid();
}

uint64_t get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t) tv.tv_sec * 1000000 + (uint64_t) tv.tv_usec;
}

#endif

void *Realloc(void *ptr, size_t sz)
{
    if (sz) return realloc(ptr, sz);
    free(ptr);
    return NULL;
}

int Fclose(FILE *f)
{
    return f ? fclose(f) : 0;
}

size_t Strnlen(const char *str, size_t len)
{
    char *end = memchr(str, '\0', len);
    return end ? (size_t) (end - str) : len;
}

// Similar to BSD 'strchrnul', but returns index instead of pointer. Index is set to length if character not found
size_t Strchrnull(const char *str, int ch)
{
    return strcspn(str, (const char [2]) { (char) ch });
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
        if (!_mm_testz_si128(a, a)) return (void *) (str + cnt + m128i_byte_scan_forward(a));
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
        if (!_mm_testz_si128(a, a)) return ( void *) (str + cnt + m128i_byte_scan_forward(a));
    }

    while (cnt) if (str[--cnt] == ch0 || str[cnt] == ch1) return ( void *) (str + cnt);
    return NULL;
}
