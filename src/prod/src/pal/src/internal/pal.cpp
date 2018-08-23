// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "pal.h"
#include "util/pal_string_util.h"
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "Common/Common.h"

using namespace std;
using namespace Pal;

#define DUMMY_HEAP  0x01020304

//LINUXTODO move exitProcessCalled to libFabricCommon.so to truly avoid racing calls to exit()
static volatile bool exitProcessCalled = false;

typedef enum _TimeConversionConstants
{
    tccSecondsToMillieSeconds       = 1000,         // 10^3
    tccSecondsToMicroSeconds        = 1000000,      // 10^6
    tccSecondsToNanoSeconds         = 1000000000,   // 10^9
    tccMillieSecondsToMicroSeconds  = 1000,         // 10^3
    tccMillieSecondsToNanoSeconds   = 1000000,      // 10^6
    tccMicroSecondsToNanoSeconds    = 1000,         // 10^3
    tccSecondsTo100NanoSeconds      = 10000000,     // 10^7
    tccMicroSecondsTo100NanoSeconds = 10            // 10^1
} TimeConversionConstants;

static bool isquote(char c)
{
    return (c == '"');
}

static int ForkExec(const char* pAppName, const char* pCmdline, const char* pWorkdir, const char* pEnvironment, int *fdin, int* fdout)
{
    // Note: space and quotation are the recognized special characters as separator.
    string appName = (pAppName? pAppName : "");
    string cmdline = (pCmdline? pCmdline : "");
    string workdir = (pWorkdir? pWorkdir : "");

    // parse executable from cmdline
    int start = 0, end = 0, argv1;
    bool quoted = false;
    for(; start < cmdline.length() && isspace(cmdline[start]); start++);
    if (isquote(cmdline[start]))
    {
        quoted = true; start++;
    }
    for(end = start; end < cmdline.length() && ((quoted && !isquote(cmdline[end])) || (!quoted && !isspace(cmdline[end]))); end++);

    if (appName.empty())
    {
        appName = cmdline.substr(start, end - start);
    }
    argv1 = (end < cmdline.length() ? end + 1 : end);
    std::replace(appName.begin(), appName.end(), '\\', '/');

    // check executable file
    struct stat stat_data;
    if (access(appName.c_str(), F_OK) != 0 || stat(appName.c_str(), &stat_data) != 0)
    {
        SetLastError(ERROR_NOT_FOUND);
        return FALSE;
    }
    if((stat_data.st_mode & S_IFMT) == S_IFDIR || (stat_data.st_mode & S_IXUSR) == 0)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    // prepare argv
    int argbuf_sz = appName.length() + cmdline.length() + 2;
    int argv_sz = std::count(cmdline.begin(), cmdline.end(), ' ') + 2;
    char** argv  = new char*[argv_sz];
    char *argbuf = new char[argbuf_sz];
    memset(argbuf, 0, argbuf_sz);

    memcpy(argbuf, appName.c_str(), appName.length());
    argbuf[appName.length()] = 0;
    int idx = appName.length() + 1;
    memcpy(argbuf + idx, cmdline.c_str() + argv1, cmdline.length() - argv1);

    argv[0] = argbuf;
    int argnum = 1;
    while (argbuf[idx])
    {
        if (isspace(argbuf[idx]))
        {
            idx++; continue;
        }

        argv[argnum++] = argbuf + idx;
        for (; argbuf[idx] && !isspace(argbuf[idx]); idx++)
        {
            if (isquote(argbuf[idx]) && (argbuf[idx - 1] != '\\'))  // skip quotation pair
            {
                for (idx++; argbuf[idx] && !(isquote(argbuf[idx]) && (argbuf[idx - 1] != '\\')); idx++);
            }
        }
        if (argbuf[idx])
        {
            argbuf[idx++] = 0;
        }
    }
    argv[argnum] = 0;

    // remove ", replace \" with "
    for (int i = 1; i < argnum; i++)
    {
        char *p = argv[i];
        int slow = 0, fast = 0;
        for (; p[fast]; fast++)
        {
            if (!isquote(p[fast]))
            {
                if (p[fast] == '\\' && isquote(p[fast + 1]))
                {
                    fast++;
                }
                p[slow++] = p[fast];
            }
        }
        p[slow] = 0;
    }

    // prepare env variable
    int envnum = 0;
    const char* lpEnv = pEnvironment;
    for (int i = 0; lpEnv && lpEnv[i]; i++)
    {
        envnum++;
        for (; lpEnv[i]; i++);
    }
    char** env = new char*[envnum + 1];
    int envidx = 0;
    for (int i = 0; lpEnv && lpEnv[i]; i++)
    {
        env[envidx++] = (char*)lpEnv + i;
        for (; lpEnv[i]; i++);
    }
    env[envnum] = 0;

    // prepare pipe
    if (fdin) pipe(fdin);
    if (fdout) pipe(fdout);

    Common::ProcessWait::Setup(); // must be called before first process creation

    int procid = fork();
    if (procid == -1)
    {
        SetLastError(ERROR_INTERNAL_ERROR);
    }
    else if (procid == 0)
    {
        if (fdin)
        {
            close(fdin[1]);  dup2(fdin[0], STDIN_FILENO);  close(fdin[0]);
        }
        if (fdout)
        {
            close(fdout[0]);  dup2(fdout[1], STDOUT_FILENO);  dup2(fdout[1], STDERR_FILENO);  close(fdout[1]);
        }

        if (!workdir.empty())
        {
            chdir(workdir.c_str());
        }
        execve(appName.c_str(), argv, env);
        ::ExitProcess(-1);
    }

    if (fdin) close(fdin[0]);
    if (fdout) close(fdout[1]);

    if (procid == -1)
    {
        if (fdin) close(fdin[1]);
        if (fdout) close(fdout[0]);
    }

    delete[] argv;
    delete[] argbuf;
    delete[] env;

    return procid;
}

PALIMPORT
BOOL
PALAPI
PAL_IsDebuggerPresent(VOID)
{
    return FALSE;
}

PALIMPORT
DWORD
PALAPI
GetEnvironmentVariableW(
            IN LPCWSTR lpName,
            OUT LPWSTR lpBuffer,
            IN DWORD nSize)
{
    string name = utf16to8(lpName);
    char* value = getenv(name.c_str());
    if (value)
    {
        wstring valueW = utf8to16(value);
        int len = valueW.length();
        if (nSize < len + 1)
        {
            return len + 1;
        }
        memcpy(lpBuffer, valueW.c_str(), (len + 1) * sizeof(wchar_t));
        return len;
    }
    return 0;
}

PALIMPORT int __cdecl _stricmp(const char *s1, const char *s2)
{
    return strcasecmp(s1, s2);
}

PALIMPORT
HANDLE
PALAPI
GetStdHandle(
         IN DWORD nStdHandle)
{
    HANDLE h = INVALID_HANDLE_VALUE;
    if (nStdHandle == STD_INPUT_HANDLE)
    {
        h = (HANDLE)0;
    }
    else if (nStdHandle == STD_OUTPUT_HANDLE)
    {
        h = (HANDLE)1;
    }
    else if (nStdHandle == STD_ERROR_HANDLE)
    {
        h = (HANDLE)2;
    }
    return h;
}

PALIMPORT
DWORD
PALAPI
GetCurrentProcessId(
            VOID)
{
    return (DWORD)getpid();
}

PALIMPORT
HANDLE
PALAPI
GetCurrentProcess(
          VOID)
{
    return (HANDLE)GetCurrentProcessId();
}

// 1269
PALIMPORT
DWORD
PALAPI
GetCurrentThreadId(
           VOID)
{
    pid_t tid = (pid_t) syscall(__NR_gettid);
    return (DWORD)tid;
}

PALIMPORT
BOOL
PALAPI
CreateProcessW(
           IN LPCWSTR lpApplicationName,
           IN LPWSTR lpCommandLine,
           IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
           IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
           IN BOOL bInheritHandles,
           IN DWORD dwCreationFlags,
           IN LPVOID lpEnvironment,
           IN LPCWSTR lpCurrentDirectory,
           IN LPSTARTUPINFOW lpStartupInfo,
           OUT LPPROCESS_INFORMATION lpProcessInformation)
{
    string appName = (lpApplicationName == NULL ? "" : utf16to8(lpApplicationName));
    string cmdline = (lpCommandLine == NULL ? "" : utf16to8(lpCommandLine));
    string workdir = (lpCurrentDirectory == NULL ? "" : utf16to8(lpCurrentDirectory));

    int procid = ForkExec(appName.c_str(), cmdline.c_str(), workdir.c_str(), (const char*)lpEnvironment, 0, 0);

    if (procid < 0)
    {
        return FALSE;
    }

    if (lpProcessInformation)
    {
        lpProcessInformation->hProcess = (HANDLE)procid;
        lpProcessInformation->hThread = 0;
        lpProcessInformation->dwProcessId = procid;
    }

    return TRUE;
}

PALIMPORT
PAL_NORETURN
VOID
PALAPI
ExitProcess(
        IN UINT uExitCode)
{
    //exit() uses a global variable that is not protected, so it is not thread-safe.
    if (!__sync_val_compare_and_swap(&exitProcessCalled, false, true))
    {
        exit(uExitCode);
    }
    else
    {
        // exit is already called, it cannot be called again as it is not MT-safe
        // need to make sure this function never returns
        for(;;)
        {
            pause();
        }
    }
}

PALIMPORT
BOOL
PALAPI
TerminateProcess(
         IN HANDLE hProcess,
         IN UINT uExitCode)
{
    DWORD procId = (DWORD)hProcess;
    if (procId == GetCurrentProcessId())
    {
        ::ExitProcess(uExitCode);
    }

    if (kill(procId, SIGKILL) == 0)
    {
        return TRUE;
    }
    switch (errno)
    {
        case ESRCH:
            SetLastError(ERROR_INVALID_HANDLE);
            break;
        case EPERM:
            SetLastError(ERROR_ACCESS_DENIED);
            break;
        default:
            SetLastError(ERROR_INTERNAL_ERROR);
            break;
    }
    return FALSE;
}

PALIMPORT
VOID
PALAPI
Sleep(
      IN DWORD dwMilliseconds)
{
    struct timespec tim;

    if (0 != clock_gettime(CLOCK_MONOTONIC, &tim)) return;

    tim.tv_sec += dwMilliseconds / 1000;
    tim.tv_nsec += (dwMilliseconds % 1000) * 1000000;
    if (tim.tv_nsec >= tccSecondsToNanoSeconds)
    {
        tim.tv_nsec -= tccSecondsToNanoSeconds;
        tim.tv_sec++;
    }
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tim, 0)  == EINTR);
    return;
}


PALIMPORT
DWORD
PALAPI
SleepEx(
    IN DWORD dwMilliseconds,
    IN BOOL bAlertable)
{
    Sleep(dwMilliseconds);
    return 0;
}

// 1497
PALIMPORT
HANDLE
PALAPI
CreateThread(
         IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
         IN DWORD dwStackSize,
         IN LPTHREAD_START_ROUTINE lpStartAddress,
         IN LPVOID lpParameter,
         IN DWORD dwCreationFlags,
         OUT LPDWORD lpThreadId)
{
    const int DefaultStackSize = 256 * 1024;
    pthread_t pthread;
    pthread_attr_t attr;
    size_t def_stacksize;

    if (pthread_attr_init(&attr) != 0)
    {
        SetLastError(ERROR_INTERNAL_ERROR);
        return 0;
    }

    if (0 == dwStackSize)
    {
        dwStackSize = DefaultStackSize;
    }
    if (pthread_attr_getstacksize(&attr, &def_stacksize) == 0)
    {
        if (def_stacksize < dwStackSize)
        {
            pthread_attr_setstacksize(&attr, dwStackSize);
        }
    }

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0
        || pthread_create(&pthread, &attr, (void *(*)(void *))lpStartAddress, lpParameter) != 0)
    {
        pthread_attr_destroy(&attr);
        SetLastError(ERROR_INTERNAL_ERROR);
        return 0;
    }

    if (lpThreadId)
    {
        *lpThreadId = (DWORD)pthread;
    }

    pthread_attr_destroy(&attr);

    return (HANDLE)pthread;
}

PALIMPORT
BOOL
PALAPI
DisableThreadLibraryCalls(
    IN HMODULE hLibModule)
{
    UNREFERENCED_PARAMETER(hLibModule);
}

// 2649
PALIMPORT
DWORD
PALAPI
GetModuleFileNameW(
    IN HMODULE hModule,
    OUT LPWSTR lpFileName,
    IN DWORD nSize)
{
    char exeName[PATH_MAX] = {0};

    int copied = 0;
    int res = readlink("/proc/self/exe", exeName, sizeof(exeName) - 1);
    if(res != -1) {
        exeName[res] = 0;
        wstring exeNameW = utf8to16(exeName);
        copied = std::min((int)(nSize - 1), res);
        memcpy(lpFileName, exeNameW.c_str(), (copied + 1) * sizeof(wchar_t));
        if (copied < res)
        {
            lpFileName[nSize - 1] = 0;
            copied = nSize;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
        return copied;
    }
    return 0;
}

// 2771
PALIMPORT
HANDLE
PALAPI
GetProcessHeap(
           VOID)
{
    return (HANDLE)DUMMY_HEAP;
}

PALIMPORT
LPVOID
PALAPI
HeapAlloc(
      IN HANDLE hHeap,
      IN DWORD dwFlags,
      IN SIZE_T numberOfBytes)
{
    char* mem;
    mem = (char *) malloc(numberOfBytes);
    if (mem == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    if (dwFlags == HEAP_ZERO_MEMORY)
    {
        memset(mem, 0, numberOfBytes);
    }
    return (mem);
}

PALIMPORT
BOOL
PALAPI
HeapFree(
     IN HANDLE hHeap,
     IN DWORD dwFlags,
     IN LPVOID lpMem)
{
    if (lpMem)
    {
        free (lpMem);
    }
    return TRUE;
}

PALIMPORT
BOOL
PALAPI
HeapSetInformation(
        IN OPTIONAL HANDLE HeapHandle,
        IN HEAP_INFORMATION_CLASS HeapInformationClass,
        IN PVOID HeapInformation,
        IN SIZE_T HeapInformationLength)
{
    // The usage of this function in service fabric is to set HeapEnableTerminationOnCorruption.
    // Make it noop on Linux.
    return TRUE;
}

PALIMPORT
HANDLE
PALAPI
OpenProcess(
    IN DWORD dwDesiredAccess, /* PROCESS_DUP_HANDLE or PROCESS_ALL_ACCESS */
    IN BOOL bInheritHandle,
    IN DWORD dwProcessId
    )
{
    return (HANDLE)dwProcessId;
}

// 3079
PALIMPORT
VOID
PALAPI
DebugBreak(
       VOID)
{
    return;
}

// 3117
PALIMPORT
BOOL
PALAPI
SetEnvironmentVariableW(
            IN LPCWSTR lpName,
            IN LPCWSTR lpValue)
{
    if (!lpValue)
    {
        unsetenv(utf16to8(lpName).c_str());
        return TRUE;
    }

    string nvpair = utf16to8(lpName) + "=" + utf16to8(lpValue);
    char *buf = (char*) malloc(nvpair.length() + 1);
    memcpy(buf, nvpair.c_str(), nvpair.length() + 1);
    return putenv(buf) == 0;
}

PALIMPORT
BOOL
PALAPI
CloseHandle(
        IN OUT HANDLE hObject)
{
    return TRUE;
}

PALIMPORT
VOID
PALAPI
RaiseException(
           IN DWORD dwExceptionCode,
           IN DWORD dwExceptionFlags,
           IN DWORD nNumberOfArguments,
           IN CONST ULONG_PTR *lpArguments)
{
    abort();
}

PALIMPORT
DWORD
PALAPI
GetTickCount(
         VOID)
{
    return (DWORD)GetTickCount64();
}

PALIMPORT
ULONGLONG
PALAPI
GetTickCount64(VOID)
{
    ULONGLONG retval = 0;

    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        goto EXIT;
    }
    retval = ((ULONGLONG)ts.tv_sec * tccSecondsToMillieSeconds) + (ts.tv_nsec / tccMillieSecondsToNanoSeconds);

EXIT:
    return retval;
}

PALIMPORT
BOOL
PALAPI
QueryPerformanceCounter(
    OUT LARGE_INTEGER *lpPerformanceCount
    )
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0)
    {
        std::string err = "clock_gettime(CLOCK_MONOTONIC) failed with error code ";
        err += std::to_string(errno);
        ASSERT(err.c_str());
        return FALSE;
    }

    lpPerformanceCount->QuadPart = (LONGLONG)ts.tv_sec * tccSecondsToNanoSeconds + (LONGLONG)ts.tv_nsec;

    return TRUE;
}

PALIMPORT
BOOL
PALAPI
QueryPerformanceFrequency(
    OUT LARGE_INTEGER *lpFrequency
    )
{
    lpFrequency->QuadPart = tccSecondsToNanoSeconds;
    return TRUE;
}

// 3702
PALIMPORT
DWORD
PALAPI
GetLastError(
         VOID)
{
    return errno;
}

PALIMPORT
VOID
PALAPI
SetLastError(
         IN DWORD dwErrCode)
{
    errno = dwErrCode;
}

PALIMPORT errno_t __cdecl memcpy_s(void *dest, size_t destsz, const void *src, size_t count)
{
    // Following the spec: http://en.cppreference.com/w/c/string/byte/memcpy we must check for the following errors:
    // - dest or src is a null pointer
    // - destsz or count is greater than RSIZE_MAX (below we don't explictly check for this because no overflows should be possible as no indexing is performed)
    // - count is greater than destsz (buffer overflow would occur)
    // - the source and the destination objects overlap

    if (dest == NULL || src == NULL)
    {
        return EFAULT;
    }

    if (destsz < 0 || count < 0)
    {
        return EINVAL;
    }

    if (count > destsz)
    {
        return ERANGE;
    }

    // In the PAL implementation, an assert is used below
    // IMO (dadmello) the bounds check needs to be present in all compiled versions
    // And thus instead of aborting we simply return an appropriate error
    UINT_PTR x = (UINT_PTR)dest, y = (UINT_PTR)src;
    if ((y + count) > x && (x + count) > y)
    {
        return EINVAL;
    }

    memcpy(dest, src, count);

    return 0;
}

PALIMPORT int __cdecl swprintf(WCHAR *s, size_t n, const WCHAR *format, ...)
{
    va_list arg;
    int r;

    va_start(arg, format);
    if (sizeof(wchar_t) == 4)
    {
        r = vswprintf(s, n, format, arg);
    }
    else
    {
        char *buf = new char[n];
        string formatA = utf16to8(format);
        r = vsnprintf(buf, n, formatA.c_str(), arg);
        if (r > 0)
        {
            wstring sW = utf8to16(buf);
            int count = sW.length();
            memcpy(s, sW.c_str(), count * sizeof(wchar_t));
            s[count] = 0;
        }
        delete[] buf;
    }
    va_end(arg);
    return r;
}

PALIMPORT int __cdecl _wtoi(const WCHAR *s)
{
    string sa = utf16to8(s);
    return atoi(sa.c_str());
}
