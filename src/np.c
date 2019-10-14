#include "np.h"
#include "ll.h"

#include <string.h>
#include <immintrin.h>

#if defined _MSC_BUILD || defined __MINGW32__

void *Aligned_alloc(size_t al, size_t sz)
{
    return _aligned_malloc(sz, al);
}

void Aligned_free(void *ptr)
{
    _aligned_free(ptr);
}

int Fseeki64(FILE *file, int64_t offset, int origin)
{
    return _fseeki64(file, offset, origin);
}

int64_t Ftelli64(FILE *file)
{
    return _ftelli64(file);
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

#elif defined __GNUC__ || defined __clang__
#   include <stdlib.h>
#   include <strings.h>

void *Aligned_alloc(size_t al, size_t sz)
{
    void *res;
    int code = posix_memalign(&res, al, sz);
    if (code) errno = code;
    return res;
}

void Aligned_free(void *ptr)
{
    free(ptr);
}

#   include <sys/types.h>
#   include <sys/stat.h>
#   include <fcntl.h>

int Fseeki64(FILE *file, int64_t offset, int origin)
{
    return fseeko(file, (off_t) offset, origin);
}

int64_t Ftelli64(FILE *file)
{
    return (int64_t) ftello(file);
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

#endif

#ifdef _WIN32
#   include <windows.h>
#   include <io.h>

bool file_is_tty(FILE *f)
{
    return _isatty(_fileno(f));
}

int64_t file_get_size(FILE *f)
{
    LARGE_INTEGER sz;
    if (GetFileSizeEx((HANDLE) _get_osfhandle(_fileno(f)), &sz)) return (int64_t) sz.QuadPart;
    else return 0;
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
    return ((uint64_t) ft.dwHighDateTime << (sizeof(uint32_t) * CHAR_BIT) | (uint64_t) ft.dwLowDateTime) / 10;
}

#elif defined __unix__ || defined __APPLE__

#   include <sys/time.h>
#   include <unistd.h>

bool file_is_tty(FILE *f)
{
    return isatty(fileno(f));
}

int64_t file_get_size(FILE *f)
{
    struct stat st;
    if (!fstat(fileno(f), &st)) return (int64_t) st.st_size;
    else return 0;
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

bool aligned_alloca_chk(size_t cnt, size_t sz, size_t al)
{
    return size_mul_add_test(&cnt, sz, al - 1);
}

int Fclose(FILE *file)
{
    return file && file != stderr && file != stdin && file != stdout ? fclose(file) : 0;
}

size_t Strnlen(const char *str, size_t len)
{
    char *end = memchr(str, '\0', len);
    return end ? (size_t) (end - str) : len;
}

size_t Strchrnul(const char *str, int ch)
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
