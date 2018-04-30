#include "np.h"

#include <string.h>

#if defined _MSC_BUILD

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

Errno_t Strerror_s(char *buff, size_t buff_sz, Errno_t code)
{
    return strerror_s(buff, buff_sz, code);
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

#include <sys/types.h>

int Fseeki64(FILE *file, int64_t offset, int origin)
{
    return fseeko(file, (off_t) offset, origin);
}

int64_t Ftelli64(FILE *file)
{
    return (int64_t) ftello(file);
}

Errno_t Strerror_s(char *buff, size_t buff_sz, Errno_t code)
{
    return strerror_r(code, buff, buff_sz);
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

size_t Strnlen(const char *str, size_t len)
{
    char *end = memchr(str, '\0', len);
    return end ? (size_t) (end - str) : len;
}

#ifdef _WIN32
#   include <windows.h>
#   include <io.h>

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
    return ((uint64_t) ft.dwHighDateTime << 32 | (uint64_t) ft.dwLowDateTime) / 10;
}

#elif defined __unix__ || defined __APPLE__
#   include <sys/types.h>
#   include <sys/time.h>
#   include <sys/stat.h>
#   include <unistd.h>

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
