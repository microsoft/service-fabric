#include <unistd.h>
#include <uuid/uuid.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/times.h>
#include <malloc.h>
#include <errno.h>
#include <wchar.h>
#include <string>
#include <algorithm>
#include <codecvt>
#include <locale>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <KXMApi.h>
#include <palio.h>
#include "ktlpal.h"
#include "palhandle.h"
#include <aio.h>
#include "ktl.h"
#include <kinternal.h>



#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)

//
// Default trace level
// TODO: Add api to enable this dynamically
//
ULONG Microsoft_Windows_KTLEnableBits[1] = { 0x1a8 }; 

#define FAIL_UNIMPLEMENTED(funcname) KInvariant(FALSE)

#define WINBASEAPI extern "C"

#if !__has_builtin(__int2c)
#ifdef __aarch64__
WINBASEAPI
VOID
WINAPI __int2c(VOID)
{
    __builtin_debugtrap();
}
#else
WINBASEAPI
VOID
WINAPI __int2c(VOID)
{
    __asm__ __volatile__("int $0x2c");
}
#endif
#endif

using namespace std;

//** Global new Ktl PAL defs
#define __thread __declspec( thread )

static const int    _g_MaxTlsSlots = 256;
static const ULONG  _g_KPalTag = 'laPK';

static void ReleaseKxmHandle();

//* Per PAL Thread state
class KPalThreadInfo
{
public:
    HANDLE              _kxmHandle;             // Set when thread has been registered with kxm IOCP
    HANDLE              _kxmIocpHandle;         // Effectively one per-thread pool

    void*               _tlsSlotValues[_g_MaxTlsSlots];

    static __thread KPalThreadInfo* _tls_CurrentThreadInfo;

    KPalThreadInfo()
    {
        // clear thread state
        _kxmHandle = INVALID_HANDLE_VALUE;
        _kxmIocpHandle = INVALID_HANDLE_VALUE;

        for (int ix = 0; ix < _g_MaxTlsSlots; ix++)
        {
            _tlsSlotValues[ix] = nullptr;
        }
    }
};
__thread KPalThreadInfo* KPalThreadInfo::_tls_CurrentThreadInfo = nullptr;
static __thread DWORD                    tls_Win32LastError = 0;

IHandleObject::IHandleObject() {}

IHandleObject::~IHandleObject()
{
    if (_kxmHandle != INVALID_HANDLE_VALUE)
    {
        ReleaseKxmHandle();
    }
}

//* KxmHandle sharing support
static HANDLE           _g_KxmHandle = INVALID_HANDLE_VALUE;
static long             _g_KxmHandleRefCount = 0;
static KSpinLock        _g_KxmHandleLock;

HANDLE AcquireKxmHandle()
{
    K_LOCK_BLOCK(_g_KxmHandleLock)
    {
        if (_g_KxmHandle == INVALID_HANDLE_VALUE)
        {
            KInvariant(_g_KxmHandleRefCount == 0);

            _g_KxmHandle = KXMOpenDev();
            KInvariant(_g_KxmHandle != INVALID_HANDLE_VALUE);
        }

        _g_KxmHandleRefCount++;
    }

    return _g_KxmHandle;
}

void ReleaseKxmHandle()
{
    // May have to close the KxmHandle
    HANDLE      kxmHandle = INVALID_HANDLE_VALUE;
    NTSTATUS status;

    K_LOCK_BLOCK(_g_KxmHandleLock)
    {
        _g_KxmHandleRefCount--;
        if (_g_KxmHandleRefCount == 0)
        {
            kxmHandle = _g_KxmHandle;
            _g_KxmHandle = INVALID_HANDLE_VALUE;
        }
    }

    if (kxmHandle != INVALID_HANDLE_VALUE)
    {
        status = KXMCloseDev(kxmHandle);
        KInvariant(status == STATUS_SUCCESS);
    }
}

static wstring utf8to16(const char *str)
{
    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> conv;
    u16string u16str = conv.from_bytes(str);
    basic_string<wchar_t> result;
    for(int index = 0; index < u16str.length(); index++)
    {
        result.push_back((wchar_t)u16str[index]);
    }
    return result;
}

static wstring utf8to16(const char *str, int length)
{
    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> conv;
    u16string u16str = conv.from_bytes(str, str+length);
    basic_string<wchar_t> result;
    for(int index = 0; index < u16str.length(); index++)
    {
        result.push_back((wchar_t)u16str[index]);
    }
    return result;
}

static string utf16to8(const wchar_t *wstr)
{
    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> conv;
    return conv.to_bytes((const char16_t *) wstr);
}

PALIMPORT
HANDLE
PALAPI
CreateEventW(
         IN LPSECURITY_ATTRIBUTES lpEventAttributes,
         IN BOOL bManualReset,
         IN BOOL bInitialState,
         IN LPCWSTR lpName)
{
    FAIL_UNIMPLEMENTED(__func__);
    return nullptr;
}

PALIMPORT
BOOL
PALAPI
SetEvent(
     IN HANDLE hEvent)
{
    FAIL_UNIMPLEMENTED(__func__);
}

BOOL
PALAPI
ResetEvent(
       IN HANDLE hEvent)
{
    FAIL_UNIMPLEMENTED(__func__);
}

// Event APIs
ULONG EVNTAPI EventRegister(
        IN LPCGUID ProviderId,
        PENABLECALLBACK EnableCallback,
        PVOID CallbackContext,
        OUT PREGHANDLE RegHandle
)
{
    return 0;
}

ULONG
EventUnregister(
        _In_ REGHANDLE RegHandle
)
{
    return 0;
}

VOID EventDataDescCreate(
        PEVENT_DATA_DESCRIPTOR EventDataDescriptor,
        VOID const* DataPtr,
        ULONG DataSize
)
{
    return;
}

ULONG EVNTAPI EventWrite(
        _In_ REGHANDLE RegHandle,
        _In_ PCEVENT_DESCRIPTOR EventDescriptor,
        _In_ ULONG UserDataCount,
        _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData
)
{
    return 0;
}

PALIMPORT
DWORD
PALAPI
WaitForSingleObject(
            IN HANDLE hHandle,
            IN DWORD dwMilliseconds)
{
    FAIL_UNIMPLEMENTED(__func__);
    return DWORD();
}

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

ULONGLONG GetTickCount64()
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

DWORD GetTickCount()
{
    return (DWORD)GetTickCount64();
}

NTSTATUS
NTAPI
NtQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_ PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_ PULONG ReturnLength
)
{
    FAIL_UNIMPLEMENTED(__func__);
    return NTSTATUS();
}

//------------------------------------------------------------------------------

PALIMPORT 
BOOL 
WINAPI 
CloseHandle(IN OUT HANDLE hObject)
{
    IHandleObject::SPtr closingObj = Ktl::Move(ResolveHandle<IHandleObject>(hObject));
    BOOL result = closingObj->Close();
    closingObj->Release(); // Backs out HANDLE ref count when HANDLE object was created
    return result;
}

BOOL HeapDestroy(_In_ HANDLE hHeap)
{
    return TRUE;
}

PALIMPORT
LPVOID
PALAPI
VirtualAlloc(
         IN LPVOID lpAddress,
         IN SIZE_T dwSize,
         IN DWORD flAllocationType,
         IN DWORD flProtect)
{
    KInvariant(lpAddress == nullptr);
    KInvariant(flAllocationType == MEM_COMMIT | MEM_RESERVE);
    KInvariant(flProtect == PAGE_READWRITE);

    void* addr = nullptr;
    long alignment = sysconf(_SC_PAGESIZE);
    int ret = posix_memalign(&addr, alignment, dwSize);
    if (ret < 0) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return nullptr;
    }

    memset(addr, 0, dwSize);
    return addr;
}

PALIMPORT
BOOL
PALAPI
VirtualFree(
        IN LPVOID lpAddress,
        IN SIZE_T dwSize,
        IN DWORD dwFreeType)
{
    if (lpAddress)
    {
        free (lpAddress);
    }
    return TRUE;
}

SIZE_T
NTAPI
RtlCompareMemory(
    _In_ const VOID * Source1,
    _In_ const VOID * Source2,
    _In_ SIZE_T Length
    )
{
    SIZE_T idx = 0;
    for (idx = 0; idx < Length; idx++)
    {
        if (((char*)Source1)[idx] != ((char*)Source2)[idx])
            break;
    }
    return idx;
}

PALIMPORT
VOID
PALAPI
RtlMoveMemory(
          IN PVOID Destination,
          IN CONST VOID *Source,
          IN SIZE_T Length)
{
    memmove(Destination, Source, Length);
    return;
}

PALIMPORT
VOID
PALAPI
RtlZeroMemory(
    IN PVOID Destination,
    IN SIZE_T Length)
{
    memset(Destination, 0, Length);
    return;
}

WINBASEAPI
VOID
WINAPI
InitializeSRWLock(
    OUT PSRWLOCK SRWLock
    )
{
    pthread_rwlock_t *lock = (pthread_rwlock_t*)SRWLock;
    *lock = PTHREAD_RWLOCK_INITIALIZER;
    int result = pthread_rwlock_init(lock, nullptr);
    ASSERT(result == 0);
}

WINBASEAPI
VOID
WINAPI
AcquireSRWLockExclusive(
    PSRWLOCK SRWLock
    )
{
    pthread_rwlock_t *lock = (pthread_rwlock_t*)SRWLock;
    int result = pthread_rwlock_wrlock(lock);
}

WINBASEAPI
BOOLEAN
WINAPI
TryAcquireSRWLockExclusive(
    PSRWLOCK SRWLock
    )
{
    pthread_rwlock_t *lock = (pthread_rwlock_t*)SRWLock;
    int result = pthread_rwlock_trywrlock(lock);
    return(result == 0);
}

WINBASEAPI
VOID
WINAPI
AcquireSRWLockShared(
    PSRWLOCK SRWLock
    )
{
    pthread_rwlock_t *lock = (pthread_rwlock_t*)SRWLock;
    int result = pthread_rwlock_rdlock(lock);
}

WINBASEAPI
BOOLEAN
WINAPI
TryAcquireSRWLockShared(
    PSRWLOCK SRWLock
    )
{
    pthread_rwlock_t *lock = (pthread_rwlock_t*)SRWLock;
    int result = pthread_rwlock_tryrdlock(lock);
    return(result == 0);
}

// pal missing, defined in PAL.cpp 3525
WINBASEAPI
VOID
WINAPI
ReleaseSRWLockExclusive(
    IN OUT PSRWLOCK SRWLock
    )
{
    pthread_rwlock_t *lock = (pthread_rwlock_t*)SRWLock;
    int result = pthread_rwlock_unlock(lock);
}

WINBASEAPI
VOID
WINAPI
ReleaseSRWLockShared(
    IN OUT PSRWLOCK SRWLock
    )
{
    pthread_rwlock_t *lock = (pthread_rwlock_t*)SRWLock;
    int result = pthread_rwlock_unlock(lock);
}

#define DUMMY_HEAP  0x01020304

PALIMPORT
HANDLE
PALAPI
GetProcessHeap(VOID)
{
    return (HANDLE)DUMMY_HEAP;
}

PALIMPORT
VOID 
WINAPI 
SetLastError(IN DWORD dwErrCode)
{
    tls_Win32LastError = dwErrCode;
}

// declared in pal.h, defined in PAL.cpp 5250
PALIMPORT
DWORD
PALAPI
GetCurrentThreadId(
           VOID)
{
    pid_t tid = (pid_t) syscall(__NR_gettid);
    return (DWORD)tid;
}

VOID WINAPI ExitProcess(
  _In_ UINT uExitCode
)
{
    exit(uExitCode);
}

WINBASEAPI
HANDLE
WINAPI
HeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize)
{
    return (HANDLE)DUMMY_HEAP;
}

ULONG
DbgPrintEx (
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_ PCSTR Format,
         ...)
{
#if DEBUGOUT_TO_CONSOLE
    //
    // TODO: Figure out the right place to send all of this output. Is
    //       it a separate file ? syslog ?  stderr ? Remove this when
    //       tracing is working ?
    //
    va_list args;
    va_start(args, Format);
    
    vprintf(Format, args);
    va_end(args);
#endif
    return 0;
}

PALIMPORT
LPVOID
PALAPI
HeapAlloc(
      IN HANDLE hHeap,
      IN DWORD dwFlags,
      IN SIZE_T dwBytes)
{
    char* mem;
    mem = (char *) malloc(dwBytes);
    if (mem == nullptr)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return nullptr;
    }
    if (dwFlags == HEAP_ZERO_MEMORY)
    {
        memset(mem, 0, dwBytes);
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

// -----------------------------------------------------------------------------

PALIMPORT
VOID
PALAPI
Sleep(
      IN DWORD dwMilliseconds)
{
    if (dwMilliseconds == 0)
    {
        // Special case - must yield to other threads at same priority

        sched_yield();
        return;
    }

    struct timespec tim, remain;
    tim.tv_sec = dwMilliseconds / 1000;
    tim.tv_nsec = (dwMilliseconds % 1000) * 1000000;
    while ((nanosleep(&tim, &remain) < 0) && (errno == EINTR))
    {
        tim.tv_sec = remain.tv_sec;
        tim.tv_nsec = remain.tv_nsec;
    }
    return;
}

EXTERN_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

// declared in pal_missing.h
extern "C"
BOOL EventEnabled(
  _In_ REGHANDLE RegHandle,
  _In_ PCEVENT_DESCRIPTOR EventDescriptor
)
{
    return TRUE;
}

errno_t memcpy_s(void *dest, size_t destsz, const void *src, size_t count)
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

    KMemCpySafe(dest, destsz, src, count);

    return 0;
}

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
        std::wstring exeNameW = utf8to16(exeName);
        copied = __min((int)(nSize - 1), res);
        KMemCpySafe(lpFileName, nSize * sizeof(wchar_t), exeNameW.c_str(), (copied + 1) * sizeof(wchar_t));
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

//* WIN32: Threading API

// Common top of stack thread entry and startup state 
class ThreadStartupState : public IHandleObject
{
    K_RUNTIME_TYPED(ThreadStartupState);
    K_FORCE_SHARED(ThreadStartupState);

public:
    LPTHREAD_START_ROUTINE  _lpStartAddress = nullptr;
    LPVOID                  _lpParameter = nullptr;
    KEvent*                 _startupEvent = nullptr;
    pthread_t               _pthread = (pthread_t)INVALID_HANDLE_VALUE;

    static ThreadStartupState::SPtr Create()
    {
        return _new(_g_KPalTag, KtlSystemCoreImp::TryGetDefaultKtlSystemCore()->PagedAllocator()) ThreadStartupState();
    }

    BOOL Close() override
    {
        return TRUE;
    }
};

ThreadStartupState::ThreadStartupState() {}
ThreadStartupState::~ThreadStartupState() {}

static DWORD WINAPI KPalThreadEntry(LPVOID lpThreadParameter)
{
    KPalThreadInfo              thisThreadInfo;
    ThreadStartupState::SPtr    thisThreadsStartupHandState = (ThreadStartupState*)lpThreadParameter;

    KPalThreadInfo::_tls_CurrentThreadInfo = &thisThreadInfo;   // Make this per thread state available to the Wind32 API imp
    thisThreadsStartupHandState->_startupEvent->SetEvent();
    lpThreadParameter = thisThreadsStartupHandState->_lpParameter;
    LPTHREAD_START_ROUTINE  lpStartAddress = thisThreadsStartupHandState->_lpStartAddress;
    //thisThreadsStartupHandState = nullptr;

    DWORD result = lpStartAddress(lpThreadParameter);
    if (thisThreadInfo._kxmHandle != INVALID_HANDLE_VALUE)
    {
        if(thisThreadInfo._kxmIocpHandle != INVALID_HANDLE_VALUE)
        {
            KXMUnregisterCompletionThread(thisThreadInfo._kxmHandle, thisThreadInfo._kxmIocpHandle);
            thisThreadInfo._kxmIocpHandle = INVALID_HANDLE_VALUE;
        }
        ReleaseKxmHandle();
        thisThreadInfo._kxmHandle = INVALID_HANDLE_VALUE;
    }
    return result;
}

#define CREATE_THREAD_DETACHED 0x80000000

PALIMPORT
HANDLE
PALAPI
InternalCreateThread(
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

    if (dwCreationFlags != CREATE_THREAD_DETACHED && dwCreationFlags != 0)
    {
        SetLastError(STATUS_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if (pthread_attr_init(&attr) != 0)
    {
        SetLastError(STATUS_INTERNAL_ERROR);
        return INVALID_HANDLE_VALUE;
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
    if (dwCreationFlags == CREATE_THREAD_DETACHED && pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
    {
        pthread_attr_destroy(&attr);
        SetLastError(STATUS_INTERNAL_ERROR);
        return INVALID_HANDLE_VALUE;
    }

    if(pthread_create(&pthread, &attr, (void *(*)(void *))lpStartAddress, lpParameter) != 0)
    {
        pthread_attr_destroy(&attr);
        SetLastError(STATUS_INTERNAL_ERROR);
        return INVALID_HANDLE_VALUE;
    }

    if (lpThreadId)
    {
        *lpThreadId = (DWORD)pthread;
    }

    pthread_attr_destroy(&attr);

    return (HANDLE)pthread;
}

PALIMPORT
VOID
PALAPI
WaitThread(HANDLE hThread)
{
    ThreadStartupState::SPtr threadHandleState = ResolveHandle<ThreadStartupState>(hThread);
    pthread_join(threadHandleState->_pthread, 0);
    return;
}

//** TODO:  To be merged with current PAL CreateThread imp
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
    ThreadStartupState::SPtr newThreadHandleState = ThreadStartupState::Create();
    if (newThreadHandleState == nullptr)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    KEvent startupSyncEvent(FALSE, FALSE);
    newThreadHandleState->_startupEvent = &startupSyncEvent;
    newThreadHandleState->_lpParameter = lpParameter;
    newThreadHandleState->_lpStartAddress = lpStartAddress;

    HANDLE actualHandle = InternalCreateThread(lpThreadAttributes, dwStackSize, KPalThreadEntry, newThreadHandleState.RawPtr(), dwCreationFlags, lpThreadId);
    if (actualHandle == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }
    newThreadHandleState->_pthread = (pthread_t)actualHandle;
    startupSyncEvent.WaitUntilSet();                // Wait for thread to start and sync

    // NOTE: CloseHandle() on return HANDLE will cause deletion of ThreadStartupState
    return (HANDLE)newThreadHandleState.Detach();               // See CloseHandle() - Returned HANDLE has ref count held for it
}


//*** (New) Win32 APIs 

//* TLS API
static KSpinLock    _g_TlsAllocationLock;
static ULONGLONG    _g_TlsAllocationMapStorage[(_g_MaxTlsSlots + ((sizeof(ULONGLONG) * 8) - 1)) / (sizeof(ULONGLONG) * 8)];
static KBitmap      _g_TlsAllocationMap(&_g_TlsAllocationMapStorage[0], sizeof(_g_TlsAllocationMapStorage), _g_MaxTlsSlots);
static bool         _g_TlsAllocationIsFirstUse = true;

DWORD WINAPI TlsAlloc(VOID)
{
    K_LOCK_BLOCK(_g_TlsAllocationLock)
    {
        if (_g_TlsAllocationIsFirstUse)
        {
            _g_TlsAllocationMap.ClearAllBits();
            _g_TlsAllocationIsFirstUse = false;
        }

        for (ULONG ix = 0; ix < _g_MaxTlsSlots; ix++)
        {
            if (!_g_TlsAllocationMap.CheckBit(ix))
            {
                _g_TlsAllocationMap.SetBits(ix, 1);
                return ix;
            }
        }
    }

    return TLS_OUT_OF_INDEXES;
}

BOOL WINAPI TlsFree(__in DWORD dwTlsIndex)
{
    K_LOCK_BLOCK(_g_TlsAllocationLock)
    {
        if (_g_TlsAllocationIsFirstUse)
        {
            _g_TlsAllocationMap.ClearAllBits();
            _g_TlsAllocationIsFirstUse = false;
        }

        if (!_g_TlsAllocationMap.CheckBit(dwTlsIndex))
        {
            return FALSE;
        }

        _g_TlsAllocationMap.ClearBits(dwTlsIndex, 1);
    }

    return TRUE;
}

BOOL WINAPI TlsSetValue(__in DWORD dwTlsIndex, __in_opt LPVOID lpTlsValue)
{
    if ((dwTlsIndex >= _g_MaxTlsSlots) || (KPalThreadInfo::_tls_CurrentThreadInfo == nullptr))
    {
        return FALSE;
    }

    KPalThreadInfo::_tls_CurrentThreadInfo->_tlsSlotValues[dwTlsIndex] = lpTlsValue;
    return TRUE;
}

LPVOID WINAPI TlsGetValue(__in DWORD dwTlsIndex)
{
    if ((dwTlsIndex >= _g_MaxTlsSlots) || (KPalThreadInfo::_tls_CurrentThreadInfo == nullptr))
    {
        return nullptr;
    }

    return KPalThreadInfo::_tls_CurrentThreadInfo->_tlsSlotValues[dwTlsIndex];
}

HANDLE WINAPI TlsGetKxmHandle()
{
    if (KPalThreadInfo::_tls_CurrentThreadInfo == nullptr) {
        return nullptr;
    }

    if (KPalThreadInfo::_tls_CurrentThreadInfo->_kxmHandle == INVALID_HANDLE_VALUE) {
        BOOL result = TlsSetupKxmHandle();
        KInvariant(result);
    }

    return KPalThreadInfo::_tls_CurrentThreadInfo->_kxmHandle;
}

BOOL WINAPI TlsSetupKxmHandle()
{
    KPalThreadInfo& thisThreadInfo = *KPalThreadInfo::_tls_CurrentThreadInfo;

    if (thisThreadInfo._kxmHandle == INVALID_HANDLE_VALUE) {
        thisThreadInfo._kxmHandle = AcquireKxmHandle();
    }

    return TRUE;
}


//** WIN32: IO Completion Port API (Partial)
class IocpHandle : public IHandleObject
{
    K_RUNTIME_TYPED(IocpHandle);
    K_FORCE_SHARED(IocpHandle);

public:
    HANDLE      _kxmIocpHandle = INVALID_HANDLE_VALUE;

    static IocpHandle::SPtr Create()
    {
        return _new(_g_KPalTag, KtlSystemCoreImp::TryGetDefaultKtlSystemCore()->PagedAllocator()) IocpHandle();
    }

    BOOL Close() override
    {
        bool result;
        if (_kxmIocpHandle != INVALID_HANDLE_VALUE)
        {
            result = KXMCloseIoCompletionPort(_kxmHandle, _kxmIocpHandle);
            KInvariant(result == true);
            _kxmIocpHandle = INVALID_HANDLE_VALUE;
        }
        return TRUE;
    }
};

IocpHandle::IocpHandle() {}
IocpHandle::~IocpHandle()
{
    KInvariant(_kxmIocpHandle == INVALID_HANDLE_VALUE);         // Not CLOSED
}

HANDLE WINAPI CreateIoCompletionPort(
    _In_     HANDLE      FileHandle,
    _In_opt_ HANDLE      ExistingCompletionPort,
    _In_     ULONG_PTR   CompletionKey,
    _In_     DWORD       NumberOfConcurrentThreads)
{
    HANDLE hdl;
    IocpHandle::SPtr iocpObj = IocpHandle::Create();

    if (iocpObj == nullptr)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    iocpObj->_kxmHandle = AcquireKxmHandle();
    KInvariant(iocpObj->_kxmHandle != INVALID_HANDLE_VALUE);

    if (ExistingCompletionPort != NULL)
    {
        ExistingCompletionPort = ResolveHandle<IocpHandle>(ExistingCompletionPort)->_kxmIocpHandle;
    }

    hdl = FileHandle == INVALID_HANDLE_VALUE ? INVALID_HANDLE_VALUE : ((NtFileHandle *)FileHandle)->GetKXMFileDescriptor();

    iocpObj->_kxmIocpHandle = KXMCreateIoCompletionPort(iocpObj->_kxmHandle,
                                                        hdl,
                                                        ExistingCompletionPort,
                                                        CompletionKey,
                                                        NumberOfConcurrentThreads);
    if (iocpObj->_kxmIocpHandle == INVALID_HANDLE_VALUE)
    {
        return INVALID_HANDLE_VALUE;
    }

    return (HANDLE)iocpObj.Detach();               // See CloseHandle() - Returned HANDLE has ref count held for it
}

BOOL WINAPI GetQueuedCompletionStatus(
    _In_  HANDLE       CompletionPort,
    _Out_ LPDWORD      lpNumberOfBytes,
    _Out_ PULONG_PTR   lpCompletionKey,
    _Out_ LPOVERLAPPED *lpOverlapped,
    _In_  DWORD        dwMilliseconds)
{
    IocpHandle::SPtr iocpObj = Ktl::Move(ResolveHandle<IocpHandle>(CompletionPort));

    KPalThreadInfo&     thisThreadInfo = *KPalThreadInfo::_tls_CurrentThreadInfo;
    if (thisThreadInfo._kxmHandle == INVALID_HANDLE_VALUE)
    {
        // Thread has never registered with Kxm
        thisThreadInfo._kxmHandle = AcquireKxmHandle();
        KInvariant(iocpObj->_kxmHandle != INVALID_HANDLE_VALUE);

        if(thisThreadInfo._kxmIocpHandle == INVALID_HANDLE_VALUE)
        {
            if(KXMRegisterCompletionThread(thisThreadInfo._kxmHandle, iocpObj->_kxmIocpHandle) == false)
            {
                return FALSE;
            }

            //Store iocpHandle, used while unregistering thread.
            thisThreadInfo._kxmIocpHandle = iocpObj->_kxmIocpHandle;
        }
    }

    return KXMGetQueuedCompletionStatus(thisThreadInfo._kxmHandle,
                                        iocpObj->_kxmIocpHandle,
                                        lpNumberOfBytes,
                                        lpCompletionKey,
                                        lpOverlapped,
                                        dwMilliseconds);
}

BOOL WINAPI PostQueuedCompletionStatus(
    _In_     HANDLE       CompletionPort,
    _In_     DWORD        dwNumberOfBytesTransferred,
    _In_     ULONG_PTR    dwCompletionKey,
    _In_opt_ LPOVERLAPPED lpOverlapped)
{
    IocpHandle::SPtr iocpObj = Ktl::Move(ResolveHandle<IocpHandle>(CompletionPort));
    KInvariant(_g_KxmHandle != INVALID_HANDLE_VALUE);

    return KXMPostQueuedCompletionStatus(_g_KxmHandle,
                                         iocpObj->_kxmIocpHandle,
                                         dwNumberOfBytesTransferred,
                                         dwCompletionKey,
                                         lpOverlapped);
}

PALIMPORT
DWORD
PALAPI 
GetLastError(VOID)
{
    return tls_Win32LastError;
}

PALIMPORT
VOID
PALAPI
DebugBreak(
       VOID)
{
    return;
}

HANDLE GetCurrentProcess()
{
    return (HANDLE)getpid();
}

ULONG
GetCurrentProcessorNumber (
        VOID
)
{
    return sched_getcpu();
}

BOOL
WINAPI
GetProcessAffinityMask(
        HANDLE hProcess,
        PDWORD_PTR lpProcessAffinityMask,
        PDWORD_PTR lpSystemAffinityMask
)
{
    int onlineProcs = sysconf(_SC_NPROCESSORS_ONLN);
    onlineProcs = __min(onlineProcs, 32);       // Limit to 32 procs max until >32 (including NUMA) is fully considered

    *lpProcessAffinityMask = *lpSystemAffinityMask = ((((DWORD_PTR)1) << onlineProcs) -1);
    return TRUE;
}

HANDLE
WINAPI
OpenThread(
        __in DWORD dwDesiredAccess,
        __in BOOL bInheritHandle,
        __in DWORD dwThreadId
)
{
    FAIL_UNIMPLEMENTED(__func__);
    return 0;
}

DWORD_PTR
WINAPI
SetThreadAffinityMask(
        __in HANDLE hThread,
        __in DWORD_PTR dwThreadAffinityMask
)
{
    FAIL_UNIMPLEMENTED(__func__);
    return 0;
}

BOOL
__stdcall
SetThreadPriority(
        _In_ HANDLE hThread,
        _In_ int nPriority
)
{
    FAIL_UNIMPLEMENTED(__func__);
    return 0;
}

DWORD
APIENTRY
SleepEx(
        __in DWORD dwMilliseconds,
        __in BOOL bAlertable
)
{
    Sleep(dwMilliseconds);
    return 0;
}

NTSTATUS
RtlMultiByteToUnicodeSize(
        OUT PULONG BytesInUnicodeString,
        IN PCSTR MultiByteString,
        IN ULONG BytesInMultiByteString);

NTSTATUS
RtlMultiByteToUnicodeN(
        OUT PWCH UnicodeString,
        IN ULONG MaxBytesInUnicodeString,
        OUT PULONG BytesInUnicodeString OPTIONAL,
        IN PCSTR MultiByteString,
        IN ULONG BytesInMultiByteString);

RPCRTAPI
_Must_inspect_result_
RPC_STATUS
RPC_ENTRY
UuidCreate (
    OUT ::GUID __RPC_FAR * Uuid
    )
{
    uuid_generate((unsigned char *)Uuid);
    return RPC_S_OK;
}


static const __int64 SECS_BETWEEN_1601_AND_1970_EPOCHS = 11644473600LL;
static const __int64 SECS_TO_100NS = 10000000; /* 10^7 */

BOOL
WINAPI
FileTimeToSystemTime(
    __in  CONST FILETIME *lpFileTime,
    __out LPSYSTEMTIME lpSystemTime
    )

{
    UINT64 fileTime = 0;

    fileTime = lpFileTime->dwHighDateTime;
    fileTime <<= 32;
    fileTime |= (UINT)lpFileTime->dwLowDateTime;
    fileTime -= SECS_BETWEEN_1601_AND_1970_EPOCHS * SECS_TO_100NS;

    time_t unixFileTime = 0;

    if (((INT64)fileTime) < 0)
    {
        unixFileTime =  -1 - ( ( -fileTime - 1 ) / SECS_TO_100NS );
    }
    else
    {
        unixFileTime = fileTime / SECS_TO_100NS;
    }

    struct tm * unixSystemTime = 0;
    struct tm timeBuf;

    unixSystemTime = gmtime_r(&unixFileTime, &timeBuf);
    lpSystemTime->wDay = unixSystemTime->tm_mday;
    lpSystemTime->wMonth = unixSystemTime->tm_mon + 1;
    lpSystemTime->wYear = unixSystemTime->tm_year + 1900;
    lpSystemTime->wSecond = unixSystemTime->tm_sec;
    lpSystemTime->wMinute = unixSystemTime->tm_min;
    lpSystemTime->wHour = unixSystemTime->tm_hour;
    lpSystemTime->wMilliseconds = (fileTime / 10000) % 1000;

    return TRUE;
}

/*++
Function:
  FILEUnixTimeToFileTime

Convert a time_t value to a win32 FILETIME structure, as described in
MSDN documentation. time_t is the number of seconds elapsed since
00:00 01 January 1970 UTC (Unix epoch), while FILETIME represents a
64-bit number of 100-nanosecond intervals that have passed since 00:00
01 January 1601 UTC (win32 epoch).
--*/
FILETIME FILEUnixTimeToFileTime( time_t sec, long nsec )
{
    __int64 Result;
    FILETIME Ret;

    Result = ((__int64)sec + SECS_BETWEEN_1601_AND_1970_EPOCHS) * SECS_TO_100NS +
        (nsec / 100);

    Ret.dwLowDateTime = (DWORD)Result;
    Ret.dwHighDateTime = (DWORD)(Result >> 32);

    return Ret;
}

VOID
GetSystemTimeAsFileTime(
            OUT LPFILETIME lpSystemTimeAsFileTime)
{
    struct timeval Time;

    if ( gettimeofday( &Time, NULL ) != 0 )
    {
        ASSERT("gettimeofday() failed");
        /* no way to indicate failure, so set time to zero */
        *lpSystemTimeAsFileTime = FILEUnixTimeToFileTime( 0, 0 );
    }
    else
    {
        /* use (tv_usec * 1000) because 2nd arg is in nanoseconds */
        *lpSystemTimeAsFileTime = FILEUnixTimeToFileTime( Time.tv_sec,
                                                          Time.tv_usec * 1000 );
    }
}

#define STRSAFEWORKERAPI    EXTERN_C HRESULT __stdcall

STRSAFEWORKERAPI
StringLengthWorkerW(
        _In_reads_or_z_(cchMax) STRSAFE_PCNZWCH psz,
        _In_ _In_range_(<=, STRSAFE_MAX_CCH) size_t cchMax,
        _Out_opt_ _Deref_out_range_(<, cchMax) _Deref_out_range_(<=, _String_length_(psz)) size_t* pcchLength)
{
    HRESULT hr = S_OK;
    size_t cchOriginalMax = cchMax;
    while (cchMax && (*psz != L'\0'))
    {
        psz++;
        cchMax--;
    }
    if (cchMax == 0)
    {
        // the string is longer than cchMax
        hr = STRSAFE_E_INVALID_PARAMETER;
    }
    if (pcchLength)
    {
        if (SUCCEEDED(hr))
        {
            *pcchLength = cchOriginalMax - cchMax;
        }
        else
        {
            *pcchLength = 0;
        }
    }

    return hr;
}

NTSTATUS RtlMultiByteToUnicodeSize(
        _Out_       PULONG BytesInUnicodeString,
        _In_  const CHAR   *MultiByteString,
        _In_        ULONG  BytesInMultiByteString
)
{
    wstring strW = utf8to16(MultiByteString, BytesInMultiByteString);
    *BytesInUnicodeString = strW.length() * sizeof(WCHAR);
    return S_OK;
}

NTSTATUS
RtlMultiByteToUnicodeN(
        OUT PWCH UnicodeString,
        IN ULONG MaxBytesInUnicodeString,
        OUT PULONG BytesInUnicodeString OPTIONAL,
        IN PCSTR MultiByteString,
        IN ULONG BytesInMultiByteString)
{
    wstring strW = utf8to16(MultiByteString, BytesInMultiByteString);
    if (BytesInUnicodeString)
    {
        *BytesInUnicodeString = strW.length() * sizeof(WCHAR);
    }
    int copyWChars = __min(MaxBytesInUnicodeString/sizeof(WCHAR), strW.length());
    memcpy(UnicodeString, strW.c_str(), copyWChars * sizeof(WCHAR));
    return S_OK;

}

NTSTATUS NtQueryVolumeInformationFile(
  _In_  HANDLE               FileHandle,
  _Out_ PIO_STATUS_BLOCK     IoStatusBlock,
  _Out_ PVOID                FsInformation,
  _In_  ULONG                Length,
  _In_  FS_INFORMATION_CLASS FsInformationClass
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

NTSTATUS WINAPI NtOpenDirectoryObject(
  _Out_ PHANDLE            DirectoryHandle,
  _In_  ACCESS_MASK        DesiredAccess,
  _In_  POBJECT_ATTRIBUTES ObjectAttributes
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

NTSTATUS WINAPI NtOpenSymbolicLinkObject(
  _Out_ PHANDLE            LinkHandle,
  _In_  ACCESS_MASK        DesiredAccess,
  _In_  POBJECT_ATTRIBUTES ObjectAttributes
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

NTSTATUS WINAPI NtQuerySymbolicLinkObject(
  _In_      HANDLE          LinkHandle,
  _Inout_   PUNICODE_STRING LinkTarget,
  _Out_opt_ PULONG          ReturnedLength
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

NTSTATUS WINAPI NtQueryDirectoryObject(
  _In_      HANDLE  DirectoryHandle,
  _Out_opt_ PVOID   Buffer,
  _In_      ULONG   Length,
  _In_      BOOLEAN ReturnSingleEntry,
  _In_      BOOLEAN RestartScan,
  _Inout_   PULONG  Context,
  _Out_opt_ PULONG  ReturnLength
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

BOOL WINAPI QueryPerformanceFrequency(
  _Out_ LARGE_INTEGER *lpFrequency
)
{
    long f = sysconf(_SC_CLK_TCK);
    if (f < 0) {
        return false;
    }
    lpFrequency->QuadPart = f;
    return true;
}

BOOL WINAPI QueryPerformanceCounter(
  _Out_ LARGE_INTEGER *lpPerformanceCount
)
{
    // Make sure the size of clock_t is 8 byte.
    static_assert(sizeof(clock_t) == 8);

    struct tms temp;
    clock_t ticks = times(&temp);
    if (ticks < 0) {
        return false;
    }
    lpPerformanceCount->QuadPart = ticks;
    return true;
}

PPERF_COUNTERSET_INSTANCE PerfCreateInstance(
  _In_ HANDLE  hProvider,
  _In_ LPCGUID CounterSetGuid,
  _In_ LPCWSTR szInstanceName,
  _In_ ULONG   dwInstance
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

ULONG PerfSetCounterRefValue(
  _In_ HANDLE                    hProvider,
  _In_ PPERF_COUNTERSET_INSTANCE pInstance,
  _In_ ULONG                     CounterId,
  _In_ PVOID                     lpAddr
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

ULONG PerfDeleteInstance(
  _In_ HANDLE                    hProvider,
  _In_ PPERF_COUNTERSET_INSTANCE InstanceBlock
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

ULONG PerfSetCounterSetInfo(
  _In_ HANDLE                hProvider,
  _In_ PPERF_COUNTERSET_INFO pTemplate,
  _In_ ULONG                 dwTemplateSize
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

ULONG PerfStartProviderEx(
  _In_     LPGUID                 ProviderGuid,
  _In_opt_ PPERF_PROVIDER_CONTEXT ProviderContext,
  _Out_    HANDLE                 *phProvider
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

ULONG PerfStopProvider(
  _In_ HANDLE hProvider
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

void WINAPI GetSystemInfo(
  _Out_ LPSYSTEM_INFO lpSystemInfo
)
{
    lpSystemInfo->dwPageSize = sysconf(_SC_PAGESIZE);
    // TODO: The rest of lpSystemInfo
}

NTSTATUS RtlCreateSecurityDescriptor(
  _Out_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_  ULONG                Revision
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

ULONG RtlLengthSid(
  _In_ PSID Sid
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

NTSTATUS RtlCreateAcl(
  _Out_ PACL  Acl,
  _In_  ULONG AclLength,
  _In_  ULONG AceRevision
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

NTSTATUS RtlAddAccessAllowedAce(
  _Inout_ PACL        Acl,
  _In_    ULONG       AceRevision,
  _In_    ACCESS_MASK AccessMask,
  _In_    PSID        Sid
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

NTSTATUS RtlSetDaclSecurityDescriptor(
  _Inout_  PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_     BOOLEAN              DaclPresent,
  _In_opt_ PACL                 Dacl,
  _In_opt_ BOOLEAN              DaclDefaulted
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

HANDLE WINAPI GetCurrentThread(void)
{
    FAIL_UNIMPLEMENTED(__func__);
}

BOOL WINAPI OpenThreadToken(
  _In_  HANDLE  ThreadHandle,
  _In_  DWORD   DesiredAccess,
  _In_  BOOL    OpenAsSelf,
  _Out_ PHANDLE TokenHandle
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

BOOL WINAPI OpenProcessToken(
  _In_  HANDLE  ProcessHandle,
  _In_  DWORD   DesiredAccess,
  _Out_ PHANDLE TokenHandle
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

BOOL WINAPI DuplicateTokenEx(
  _In_     HANDLE                       hExistingToken,
  _In_     DWORD                        dwDesiredAccess,
  _In_opt_ LPSECURITY_ATTRIBUTES        lpTokenAttributes,
  _In_     SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
  _In_     TOKEN_TYPE                   TokenType,
  _Out_    PHANDLE                      phNewToken
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

BOOL WINAPI SetThreadToken(
  _In_opt_ PHANDLE Thread,
  _In_opt_ HANDLE  Token
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

BOOL WINAPI AdjustTokenPrivileges(
  _In_      HANDLE            TokenHandle,
  _In_      BOOL              DisableAllPrivileges,
  _In_opt_  PTOKEN_PRIVILEGES NewState,
  _In_      DWORD             BufferLength,
  _Out_opt_ PTOKEN_PRIVILEGES PreviousState,
  _Out_opt_ PDWORD            ReturnLength
)
{
    FAIL_UNIMPLEMENTED(__func__);
}

WINADVAPI
LSTATUS
APIENTRY
RegOpenKeyExW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    )
{
    FAIL_UNIMPLEMENTED(__func__);
}

WINADVAPI
LSTATUS
APIENTRY
RegCloseKey(
    _In_ HKEY hKey
    )
{
    FAIL_UNIMPLEMENTED(__func__);
}

WINADVAPI
LSTATUS
APIENTRY
RegQueryValueExW(
    _In_ HKEY hKey,
    _In_opt_ LPCWSTR lpValueName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
     LPBYTE lpData,
     LPDWORD lpcbData
    )
{
    FAIL_UNIMPLEMENTED(__func__);
}

WINADVAPI
LSTATUS
APIENTRY
RegSetValueExW(
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE *lpData,
    DWORD cbData
    )
{
    FAIL_UNIMPLEMENTED(__func__);
}

WINADVAPI
LSTATUS
APIENTRY
RegDeleteValueW(
    _In_ HKEY hKey,
    _In_opt_ LPCWSTR lpValueName
    )
{
    FAIL_UNIMPLEMENTED(__func__);
}

WINBASEAPI
VOID
WINAPI
SetThreadpoolTimer(
    PTP_TIMER pti,
    PFILETIME pftDueTime,
    DWORD msPeriod,
    DWORD msWindowLength
    )
{
    FAIL_UNIMPLEMENTED(__func__);
}

WINBASEAPI
VOID
WINAPI
CloseThreadpoolTimer(
    PTP_TIMER pti
    )
{
    FAIL_UNIMPLEMENTED(__func__);
}

WINBASEAPI
PTP_TIMER
WINAPI
CreateThreadpoolTimer(
    PTP_TIMER_CALLBACK pfnti,
    PVOID pv,
    PTP_CALLBACK_ENVIRON pcbe
    )
{
    FAIL_UNIMPLEMENTED(__func__);
}

WINBASEAPI
VOID
WINAPI
WaitForThreadpoolTimerCallbacks(
    PTP_TIMER pti,
    BOOL fCancelPendingCallbacks
    )
{
    FAIL_UNIMPLEMENTED(__func__);
}

NTSTATUS
RtlStringCchCatW(
    __out_ecount(cchDest) STRSAFE_LPWSTR pszDest,
    __in size_t cchDest,
    __in STRSAFE_LPCWSTR pszSrc)
{
    FAIL_UNIMPLEMENTED(__func__);
}

NTSTATUS
RtlStringCchLengthW(
    _In_      STRSAFE_PCNZWCH psz,
    _In_      size_t cchMax,
    _Out_opt_ size_t* pcchLength)
{
    FAIL_UNIMPLEMENTED(__func__);
}

NTSTATUS
RtlStringCchCopyW(
    _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
    _In_ size_t cchDest,
    _In_ NTSTRSAFE_PCWSTR pszSrc)
{
    FAIL_UNIMPLEMENTED(__func__);
}

DWORD GetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize)
{
    char* value = getenv(lpName);
    if (value)
    {
        int len = strlen(value);
        if (nSize < len + 1)
        {
            return len + 1;
        }
        KMemCpySafe(lpBuffer, nSize, value, len + 1);
        return len;
    }
    return 0;
}

DWORD GetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize)
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

bool GetEnvironmentVariable(string const & name, string& outValue)
{
    int valueLen = 16; // first shot
    outValue.resize(valueLen);
    for(;;)
    {
        int n = GetEnvironmentVariableA(name.c_str(), &outValue[0], valueLen);
        outValue.resize(n);
        if (n == 0)
        {
            return false;
        }
        else if (n < valueLen)
        {
            break;
        }
        else
        {
            valueLen = n;
        }
    }
    return true;
}

bool GetEnvironmentVariable(wstring const & name, wstring& outValue)
{
    int valueLen = 16; // first shot
    outValue.resize(valueLen);
    for(;;)
    {
        int n = GetEnvironmentVariableW(name.c_str(), &outValue[0], valueLen);
        outValue.resize(n);
        if (n == 0)
        {
            return false;
        }
        else if (n < valueLen)
        {
            break;
        }
        else
        {
            valueLen = n;
        }
    }
    return true;
}

WINBASEAPI
DWORD
WINAPI
ExpandEnvironmentStringsA(
    _In_ LPCSTR lpSrc,
    LPSTR lpDst,
    _In_ DWORD nSize)
{
    int si = 0;
    string dest;

    while(lpSrc[si])
    {
        if (lpSrc[si] != '%')
        {
            dest.push_back(lpSrc[si]);
            si ++;
        }
        else
        {
            int endtoken = si + 1;
            while(lpSrc[endtoken] && lpSrc[endtoken] != '%')  endtoken++;

            string token, tokenvalue;
            if(lpSrc[endtoken] == '%')
            {
                for (int i = si + 1; i < endtoken; i++)
                    token.push_back(lpSrc[i]);
                if (GetEnvironmentVariable(token, tokenvalue))
                    dest += tokenvalue;
                si = endtoken + 1;
            }
            else
            {
                for (int i = si + 1; i < endtoken; i++)
                    dest.push_back(lpSrc[i]);
                si = endtoken;
            }
        }
    }
    if (nSize > dest.length())
    {
        KMemCpySafe(lpDst, nSize, dest.c_str(), dest.length());
        lpDst[dest.length()] = 0;
    }
    return dest.length() + 1;
}

WINBASEAPI
DWORD
WINAPI
ExpandEnvironmentStringsW(_In_ LPCWSTR lpSrc, LPWSTR lpDst, _In_ DWORD nSize)
{
    int si = 0;
    wstring dest;

    while(lpSrc[si])
    {
        if (lpSrc[si] != '%')
        {
            dest.push_back(lpSrc[si]);
            si ++;
        }
        else
        {
            int endtoken = si + 1;
            while(lpSrc[endtoken] && lpSrc[endtoken] != '%')  endtoken++;

            wstring token, tokenvalue;
            if(lpSrc[endtoken] == '%')
            {
                for (int i = si + 1; i < endtoken; i++)
                    token.push_back(lpSrc[i]);
                if (GetEnvironmentVariable(token, tokenvalue))
                    dest += tokenvalue;
                si = endtoken + 1;
            }
            else
            {
                for (int i = si + 1; i < endtoken; i++)
                    dest.push_back(lpSrc[i]);
                si = endtoken;
            }
        }
    }
    if (nSize > dest.length())
    {
        KMemCpySafe(lpDst, nSize, dest.c_str(), sizeof(wchar_t)*dest.length());
        lpDst[dest.length()] = 0;
    }
    return dest.length() + 1;
}

ULONG RtlRandomEx(
  _Inout_ PULONG Seed
)
{
#if KTL_USER_MODE
    std::srand(*Seed);
    *Seed = std::rand() % MAXLONG;
    return *Seed;
#else
    // TODO Kernel mode
#endif
}

BOOL WINAPI VirtualProtect(
  _In_  LPVOID lpAddress,
  _In_  SIZE_T dwSize,
  _In_  DWORD  flNewProtect,
  _Out_ PDWORD lpflOldProtect
)
{
    int prot = 0;
    switch(flNewProtect) {
      case PAGE_EXECUTE: {
        prot = PROT_EXEC;
      } break;
      case PAGE_EXECUTE_READ: {
        prot = PROT_EXEC | PROT_READ;
      } break;
      case PAGE_EXECUTE_READWRITE: {
        prot = PROT_EXEC | PROT_READ | PROT_WRITE;
      } break;
      case PAGE_NOACCESS: {
        prot = PROT_NONE;
      } break;
      case PAGE_READONLY: {
        prot = PROT_READ;
      } break;
      case PAGE_READWRITE: {
        prot = PROT_READ | PROT_WRITE;
      } break;
      default:
        ASSERT(false);
        break;
    }
    int ret = mprotect(lpAddress, dwSize, prot);
    return ret == 0;
}

DWORD WINAPI WaitForMultipleObjects(
  _In_       DWORD  nCount,
  _In_ const HANDLE *lpHandles,
  _In_       BOOL   bWaitAll,
  _In_       DWORD  dwMilliseconds
)
{
    int i = 0;
    WaitThread(lpHandles[i++]);

    while (bWaitAll && i < nCount) {
        WaitThread(lpHandles[i++]);
    }

    return WAIT_OBJECT_0;
}

BOOL
SymInitialize(
        _In_ HANDLE hProcess,
        _In_opt_ PCSTR UserSearchPath,
        _In_ BOOL fInvadeProcess
)
{
    // assume the symbol is within the executable
    ASSERT(UserSearchPath == 0);
    return TRUE;
}

USHORT WINAPI CaptureStackBackTrace(
        _In_      ULONG  FramesToSkip,
        _In_      ULONG  FramesToCapture,
        _Out_     PVOID  *BackTrace,
        _Out_opt_ PULONG BackTraceHash
)
{
    int captured = backtrace(BackTrace, FramesToCapture);
    ASSERT(captured > FramesToSkip);

    if (FramesToSkip)
    {
        for (int i = FramesToSkip; i < captured; i++)
            BackTrace[i - FramesToSkip] = BackTrace[i];
    }
    return captured - FramesToSkip;
}

BOOL
SymFromAddr(
    __in HANDLE hProcess,
    __in DWORD64 Address,
    __out_opt PDWORD64 Displacement,
    __inout PSYMBOL_INFO Symbol
)
{
    BOOL found = FALSE;
    void * addrs[1] = { (void*)Address };
    char** symbols = backtrace_symbols(addrs, 1);
    Symbol->Name[0] = 0;
    if (symbols && symbols[0])
    {
        strncpy(Symbol->Name, symbols[0], Symbol->MaxNameLen);
        found = TRUE;
    }
    return found;
}

BOOL WINAPI SymGetLineFromAddr64(
    _In_  HANDLE           hProcess,
    _In_  DWORD64          dwAddr,
    _Out_ PDWORD           pdwDisplacement,
    _Out_ PIMAGEHLP_LINE64 Line
)
{
    Line->FileName = 0;
    Line->LineNumber = 0;
    return TRUE;
}

VOID __movsb (
        PUCHAR Destination,
        PCUCHAR Source,
        SIZE_T Count
)
{
    SIZE_T i;

    for (i = 0; i < Count; ++i) {
        Destination[i] = Source[i];
    }

    return;
}


ULONG RtlNtStatusToDosError( __in  NTSTATUS Status )
{
    return (ULONG) Status;
}

int _wcsicmp(const WCHAR *s1, const WCHAR*s2)
{
    return wcscasecmp(s1, s2);
}

SHORT
InterlockedCompareExchange16 (
    __inout SHORT volatile *Destination,
    __in SHORT ExChange,
    __in SHORT Comperand
    )
{
    return __sync_val_compare_and_swap(Destination, Comperand, ExChange);
}

LONGLONG
InterlockedAdd64 (
    LONGLONG volatile *lpAddend,
    LONGLONG addent
    )
{
   return __sync_add_and_fetch(lpAddend, (LONGLONG)addent);
}


CHAR InterlockedExchange8(CHAR volatile *Target, CHAR Value)
{
    return __sync_swap(Target, Value);
}


#ifdef __aarch64__ 
VOID
__stosq (
    IN PDWORD64 Destination,
    IN DWORD64 Value,
    IN SIZE_T Count
    )
{
    for(size_t i = 0 ;i < Count;i++)
    {
        Destination[i] = Value;
    }
}
#else
VOID
__stosq (
    IN PDWORD64 Destination,
    IN DWORD64 Value,
    IN SIZE_T Count
    )
{
    __asm__("rep stosq" : : "D"(Destination), "a"(Value), "c"(Count));
}
#endif


//
// Run functions
//
VOID
KtlCopyMemory (
    __out VOID UNALIGNED *Destination,
    __in CONST VOID UNALIGNED * Sources,
    __in SIZE_T Length
)
{
    KMemCpySafe(Destination, Length, Sources, Length);
}


#define MAXUSHORT   0xffff
#define MAX_USTRING ( sizeof(WCHAR) * (MAXUSHORT/sizeof(WCHAR)) )

VOID
KtlInitUnicodeString (
    __out PUNICODE_STRING DestinationString,
    __in_opt PCWSTR SourceString
)
{
    ULONG length;
    DestinationString->Buffer = (PWSTR)SourceString;
    if (ARGUMENT_PRESENT(SourceString)) {
        length = (ULONG)wcslen( SourceString ) * sizeof( WCHAR);

        KAssert(length < MAX_USTRING);

        if (length >= MAX_USTRING) {
            length = MAX_USTRING - sizeof(UNICODE_NULL);
        }

        DestinationString->Length = (USHORT)length;
        DestinationString->MaximumLength = (USHORT)(length + sizeof(UNICODE_NULL));

    } else {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
    }
}

VOID
KtlCopyUnicodeString(
    __inout PUNICODE_STRING DestinationString,
    __in_opt PCUNICODE_STRING SourceString
)
{
    UNALIGNED WCHAR *src, *dst;
    ULONG n;

    if (ARGUMENT_PRESENT(SourceString)) {
        dst = DestinationString->Buffer;
        src = SourceString->Buffer;
        n = SourceString->Length;
        if ((USHORT)n > DestinationString->MaximumLength) {
            n = DestinationString->MaximumLength;
        }

        DestinationString->Length = (USHORT)n;
        KtlCopyMemory(dst, src, n);
        if( (DestinationString->Length + sizeof (WCHAR)) <= DestinationString->MaximumLength) {
            dst[n / sizeof(WCHAR)] = UNICODE_NULL;
        }

    } else {
        DestinationString->Length = 0;
    }
}

VOID
KtlFreeUnicodeString (
   __in PUNICODE_STRING UnicodeString
)
{
    if (UnicodeString->Buffer) {
        free(UnicodeString->Buffer);
        RtlZeroMemory(UnicodeString, sizeof(*UnicodeString));
    }
}

LONG
KtlCompareUnicodeStrings (
    __in_ecount(String1Length) PCWCH String1,
    __in SIZE_T String1Length,
    __in_ecount(String2Length) PCWCH String2,
    __in SIZE_T String2Length,
    __in BOOLEAN CaseInSensitive
)
{
    PCWSTR s1, s2, Limit;
    SIZE_T n1, n2;
    WCHAR c1, c2;
    LONG returnValue;

    s1 = String1;
    s2 = String2;
    n1 = String1Length;
    n2 = String2Length;

    KAssert(!(((((ULONG_PTR)s1 & 1) != 0) || (((ULONG_PTR)s2 & 1) != 0))));

    Limit = s1 + (n1 <= n2 ? n1 : n2);
    returnValue = (LONG)(n1 - n2);
    if (CaseInSensitive) {
        while (s1 < Limit) {
            c1 = *s1++;
            c2 = *s2++;
            if (c1 != c2) {

                //
                // Note that this needs to reference the translation table!
                //

                c1 = std::toupper(c1);
                c2 = std::toupper(c2);
                if (c1 != c2) {
                    returnValue = (LONG)(c1) - (LONG)(c2);
                    break;
                }
            }
        }

    } else {
        while (s1 < Limit) {
            c1 = *s1++;
            c2 = *s2++;
            if (c1 != c2) {
                returnValue = (LONG)(c1) - (LONG)(c2);
                break;
            }
        }
    }
    return returnValue;
}


LONG
KtlCompareUnicodeString (
    __in PCUNICODE_STRING String1,
    __in PCUNICODE_STRING String2,
    __in BOOLEAN CaseInSensitive
)
{
    return KtlCompareUnicodeStrings(String1->Buffer,
                                    String1->Length / sizeof(WCHAR),
                                    String2->Buffer,
                                    String2->Length / sizeof(WCHAR),
                                    CaseInSensitive);
}

NTSTATUS
KtlAppendUnicodeStringToString (
    __inout PUNICODE_STRING Destination,
    __in PCUNICODE_STRING Source
)
{
    USHORT n;
    WCHAR UNALIGNED *dst;
    NTSTATUS status;

    status = STATUS_SUCCESS;
    n = Source->Length;
    if (n) {
        if ((n + Destination->Length) > Destination->MaximumLength) {
            status = STATUS_BUFFER_TOO_SMALL;

        } else {
            dst = &Destination->Buffer[(Destination->Length / sizeof(WCHAR))];
            RtlMoveMemory(dst, Source->Buffer, n);
            Destination->Length = Destination->Length + n;
            if (Destination->Length < Destination->MaximumLength) {
                dst[n / sizeof(WCHAR)] = UNICODE_NULL;
            }
        }
    }
    return status;
}


static
int
__cdecl
KtlScanHexFormat(
    __in_ecount(MaximumLength) const WCHAR* Buffer,
    __in ULONG MaximumLength,
    __in_z const WCHAR* Format,
    ...)
{
    va_list argList;
    int     formatItems;

    va_start(argList, Format);
    for (formatItems = 0;;) {
        switch (*Format) {
            case 0:
                return (MaximumLength && *Buffer) ? -1 : formatItems;
            case '%':
                Format++;
                if (*Format != '%') {
                    ULONG   number;
                    int     width;
                    int     long1;
                    PVOID   pointer;

                    for (long1 = 0, width = 0;; Format++) {
                        if ((*Format >= '0') && (*Format <= '9')) {
                            width = width * 10 + *Format - '0';
                        } else if (*Format == 'l') {
                            long1++;
                        } else if ((*Format == 'X') || (*Format == 'x')) {
                            if (width >= 8) long1++;
                            break;
                        }
                    }
                    Format++;
                    for (number = 0; width--; Buffer++, MaximumLength--) {
                        if (!MaximumLength)
                            return -1;
                        number *= 16;
                        if ((*Buffer >= '0') && (*Buffer <= '9')) {
                            number += (*Buffer - '0');
                        } else if ((*Buffer >= 'a') && (*Buffer <= 'f')) {
                            number += (*Buffer - 'a' + 10);
                        } else if ((*Buffer >= 'A') && (*Buffer <= 'F')) {
                            number += (*Buffer - 'A' + 10);
                        } else {
                            return -1;
                        }
                    }
                    pointer = va_arg(argList, PVOID);
                    if (long1) {
                        *(PULONG)pointer = number;
                    } else {
                        *(PUSHORT)pointer = (USHORT)number;
                    }
                    formatItems++;
                    break;
                }
                /* no break */
            default:
                if (!MaximumLength || (*Buffer != *Format)) {
                    return -1;
                }
                Buffer++;
                MaximumLength--;
                Format++;
                break;
        }
    }
}

#define GUIDFORMAT   L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"
#define GUIDFORMAT_A "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"

NTSTATUS
KtlGUIDFromString(
    __in PCUNICODE_STRING GuidString,
    __out GUID* Guid
)
{
    USHORT    data4[8] = {0};
    int       count;

    if (KtlScanHexFormat(GuidString->Buffer, GuidString->Length / sizeof(WCHAR), GUIDFORMAT,
                      &Guid->Data1, &Guid->Data2, &Guid->Data3,
                      &data4[0], &data4[1], &data4[2], &data4[3],
                      &data4[4], &data4[5], &data4[6], &data4[7]) == -1) {
        return STATUS_INVALID_PARAMETER;
    }

    for (count = 0; count < sizeof(data4)/sizeof(data4[0]); count++) {
        Guid->Data4[count] = (UCHAR)data4[count];
    }

    return STATUS_SUCCESS;
}

#define KTL_GUID_STRING_SIZE 38
NTSTATUS
KtlStringFromGUIDEx(
    __in REFGUID Guid,
    __inout PUNICODE_STRING GuidString,
    __in BOOLEAN Allocate
)
{
    const USHORT numChars = KTL_GUID_STRING_SIZE + 1;
    const USHORT requiredSize = numChars * sizeof(WCHAR);
    if (Allocate != FALSE) {
        GuidString->MaximumLength = requiredSize;
        GuidString->Buffer = (PWSTR)malloc(requiredSize);
        if (GuidString->Buffer == NULL) {
            return STATUS_NO_MEMORY;
        }

    } else if (GuidString->MaximumLength < requiredSize) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    GuidString->Length = requiredSize - sizeof(UNICODE_NULL);

    char buf[numChars] = {0};
    snprintf(  buf,
               numChars,
               GUIDFORMAT_A,
               Guid.Data1,
               Guid.Data2,
               Guid.Data3,
               Guid.Data4[0],
               Guid.Data4[1],
               Guid.Data4[2],
               Guid.Data4[3],
               Guid.Data4[4],
               Guid.Data4[5],
               Guid.Data4[6],
               Guid.Data4[7]);
    wstring res = utf8to16(buf);
    KMemCpySafe((char*)GuidString->Buffer, requiredSize, (char*)res.c_str(), res.length() * sizeof(WCHAR));
    GuidString->Buffer[res.length()] = 0;
    return STATUS_SUCCESS;
}

NTSTATUS
KtlStringFromGUID(
    __in REFGUID Guid,
    __in PUNICODE_STRING GuidString
    )
{
    return KtlStringFromGUIDEx(Guid, GuidString, TRUE);
}

#define MAX_DIGITS 65
#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS)0x80000005L)
const WCHAR KtlpIntegerWChars[] = { L'0', L'1', L'2', L'3', L'4', L'5',
                                    L'6', L'7', L'8', L'9', L'A', L'B',
                                    L'C', L'D', L'E', L'F' };
const CHAR KtlpIntegerChars[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                 '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

__inline
LARGE_INTEGER
KtlExtendedLargeIntegerDivide (
    __in LARGE_INTEGER Dividend,
    __in ULONG Divisor,
    __out_opt PULONG Remainder
)
{
    LARGE_INTEGER quotient;

    quotient.QuadPart = (ULONG64)Dividend.QuadPart / Divisor;
    if (ARGUMENT_PRESENT(Remainder)) {
        *Remainder = (ULONG)((ULONG64)Dividend.QuadPart % Divisor);
    }

    return quotient;
}

NTSTATUS
KtlLargeIntegerToUnicode (
    __in PLARGE_INTEGER Value,
    __in ULONG Base,
    __in LONG OutputLength,
    __out PWSTR String
)
{
    WCHAR result [MAX_DIGITS], *s;
    ULONG shift, mask = 0, digit, length;

    shift = 0;
    switch (Base) {
        case 16:    shift = 4;  break;
        case  8:    shift = 3;  break;
        case  2:    shift = 1;  break;
        case  0:
        case 10:    shift = 0;  break;
        default:    return (STATUS_INVALID_PARAMETER);
    }

    if (shift != 0) {
        mask = (1 << shift) - 1;
    }

    s = &result [MAX_DIGITS];
    if (shift != 0) {
        ULONGLONG tValue;

        tValue = (ULONGLONG) Value->QuadPart;
        do {
            digit  = (ULONG) (tValue & mask);
            tValue = tValue >> shift;
            *--s = KtlpIntegerWChars[digit];
        } while (tValue != 0);
    } else {
        LARGE_INTEGER tempValue = *Value;
        do {
            tempValue = KtlExtendedLargeIntegerDivide (tempValue, Base, &digit);
            *--s = KtlpIntegerWChars[digit];
        } while (tempValue.QuadPart != 0);
    }

    length = (ULONG)(&result[MAX_DIGITS] - s);

    KAssert (length <= MAX_DIGITS);

    if (OutputLength < 0) {
        OutputLength = -OutputLength;
        while ((LONG)length < OutputLength) {
            *String++ = L'0';
            OutputLength--;
        }
    }

    if ((LONG)length > OutputLength) {
        return (STATUS_BUFFER_OVERFLOW);
    }
    else {
        KtlCopyMemory (String, s, length * sizeof(WCHAR));
        if ((LONG)length < OutputLength) {
            String [length] = L'\0';
        }
        return (STATUS_SUCCESS);
    }
}

NTSTATUS
KtlLargeIntegerToChar (
    __in PLARGE_INTEGER Value,
    __in ULONG Base,
    __in LONG OutputLength,
    __out PSZ String
)
{
    CHAR result[MAX_DIGITS], *s;
    ULONG shift, mask = 0, digit, length;

    shift = 0;
    switch (Base) {
        case 16:    shift = 4;  break;
        case  8:    shift = 3;  break;
        case  2:    shift = 1;  break;

        case  0:
        case 10:    shift = 0;  break;
        default:    return (STATUS_INVALID_PARAMETER);
    }

    if (shift != 0) {
        mask = (1 << shift) - 1;
    }

    s = &result[MAX_DIGITS];

    if (shift != 0) {
        ULONGLONG tValue;

        tValue = (ULONGLONG) Value->QuadPart;
        do {
            digit  = (ULONG) (tValue & mask);
            tValue = tValue >> shift;
            *--s = KtlpIntegerChars[digit];
        } while (tValue != 0);

    } else {
        LARGE_INTEGER tempValue = *Value;
        do {
            tempValue = KtlExtendedLargeIntegerDivide (tempValue, Base, &digit);
            *--s = KtlpIntegerChars [digit];
        } while (tempValue.QuadPart != 0);
    }

    length = (ULONG)(&result[MAX_DIGITS] - s);
    if (OutputLength < 0) {
        OutputLength = -OutputLength;
        while ((LONG)length < OutputLength) {
            *String++ = '0';
            OutputLength--;
        }
    }

    if ((LONG)length > OutputLength) {
        return (STATUS_BUFFER_OVERFLOW);
    }
    else {
        KtlCopyMemory (String, s, length);
        if ((LONG)length < OutputLength) {
            String[length] = '\0';
        }
        return (STATUS_SUCCESS);
    }
}


#define STATUS_INVALID_PARAMETER_2       ((NTSTATUS)0xC00000F0L)

NTSTATUS
KtlAnsiStringToUnicodeString (
    __inout PUNICODE_STRING DestinationString,
    __in ANSI_STRING *SourceString,
    __in BOOLEAN AllocateDestinationString
)
{

    ULONG unicodeLength;
    ULONG index;
    NTSTATUS status;

    KAssert(AllocateDestinationString == FALSE);

    unicodeLength = (SourceString->Length * sizeof(WCHAR)) +
                    sizeof(UNICODE_NULL);

    if (unicodeLength > MAX_USTRING) {
        status = STATUS_INVALID_PARAMETER_2;
        return status;
    }

    if (unicodeLength > DestinationString->MaximumLength) {
        status = STATUS_BUFFER_OVERFLOW;
        return status;
    }

    DestinationString->Length = (USHORT)(unicodeLength - sizeof(UNICODE_NULL));
    index = 0;
    while(index < SourceString->Length ) {
        DestinationString->Buffer[index] = (WCHAR)SourceString->Buffer[index];
        index++;
    }

    if (DestinationString->Length < DestinationString->MaximumLength) {
        DestinationString->Buffer[index] = UNICODE_NULL;
    }

    status = STATUS_SUCCESS;

    return status;
}

NTSTATUS
KtlInt64ToUnicodeString (
    __in ULONGLONG Value,
    __in ULONG Base,
    __in PUNICODE_STRING String
)
{
    NTSTATUS status;
    UCHAR resultBuffer[32];
    ANSI_STRING ansiString;
    LARGE_INTEGER temp;

    temp.QuadPart = Value;
    status = KtlLargeIntegerToChar(&temp,
                                   Base,
                                   sizeof(resultBuffer),
                                   (PSZ)resultBuffer);

    if (NT_SUCCESS(status)) {
        ansiString.Buffer = (PSZ)resultBuffer;
        ansiString.MaximumLength = sizeof(resultBuffer);
        ansiString.Length = (USHORT)strlen((PSZ)resultBuffer);
        status = KtlAnsiStringToUnicodeString(String, &ansiString, FALSE);
    }

    return status;
}

//
// String Safe functions
//
WINBASEAPI HRESULT KtlStringCchLengthW(
    _In_  LPCWSTR psz,
    _In_  size_t  cchMax,
    _Out_ size_t  *pcch
    )
{
    HRESULT hr = S_OK;
    size_t cchMaxPrev = cchMax;

    while (cchMax && (*psz != L'\0'))
    {
        psz++;
        cchMax--;
    }

    if (cchMax == 0)
    {
        // the string is longer than cchMax
        hr = STRSAFE_E_INVALID_PARAMETER;
    }

    if (SUCCEEDED(hr) && pcch)
    {
        *pcch = cchMaxPrev - cchMax;
    }

    return hr;
}

WINBASEAPI HRESULT KtlStringCchLengthA(
    _In_  LPCSTR psz,
    _In_  size_t  cchMax,
    _Out_ size_t  *pcch
    )
{
    HRESULT hr = S_OK;
    size_t cchMaxPrev = cchMax;

    while (cchMax && (*psz != '\0'))
    {
        psz++;
        cchMax--;
    }

    if (cchMax == 0)
    {
        // the string is longer than cchMax
        hr = STRSAFE_E_INVALID_PARAMETER;
    }

    if (SUCCEEDED(hr) && pcch)
    {
        *pcch = cchMaxPrev - cchMax;
    }

    return hr;
}

STRSAFEAPI
KtlStringCopyWorkerW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc)
{
    HRESULT hr = S_OK;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        hr = STRSAFE_E_INVALID_PARAMETER;
    }
    else
    {
        while (cchDest && (*pszSrc != L'\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
        }

        if (cchDest == 0)
        {
            // we are going to truncate pszDest
            pszDest--;
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
        }

        *pszDest= L'\0';
    }

    return hr;
}

STRSAFEAPI
KtlStringCchCopyW(
        _Out_writes_(cchDest) _Always_(_Post_z_) STRSAFE_LPWSTR pszDest,
        _In_ size_t cchDest,
        _In_ STRSAFE_LPCWSTR pszSrc)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        hr = STRSAFE_E_INVALID_PARAMETER;
    }
    else
    {
        hr = KtlStringCopyWorkerW(pszDest, cchDest, pszSrc);
    }

    return hr;
}

STRSAFEAPI
KtlStringCopyWorkerA(char* pszDest, size_t cchDest, const char* pszSrc)
{
    HRESULT hr = S_OK;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        hr = STRSAFE_E_INVALID_PARAMETER;
    }
    else
    {
        while (cchDest && (*pszSrc != '\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
        }

        if (cchDest == 0)
        {
            // we are going to truncate pszDest
            pszDest--;
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
        }

        *pszDest= '\0';
    }

    return hr;
}

STRSAFEAPI
KtlStringCchCopyA(
        _Out_writes_(cchDest) _Always_(_Post_z_) STRSAFE_LPSTR pszDest,
        _In_ size_t cchDest,
        _In_ STRSAFE_LPCSTR pszSrc)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        hr = STRSAFE_E_INVALID_PARAMETER;
    }
    else
    {
        hr = KtlStringCopyWorkerA(pszDest, cchDest, pszSrc);
    }

    return hr;
}


STRSAFEAPI
KtlStringVPrintfWorkerW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszFormat, va_list argList)
{
    HRESULT hr = S_OK;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        hr = STRSAFE_E_INVALID_PARAMETER;
    }
    else
    {
        int iRet;
        size_t cchMax;

        // leave the last space for the null terminator
        cchMax = cchDest - 1;

        //iRet = _vsnwprintf(pszDest, cchMax, pszFormat, argList);
        iRet = vswprintf(pszDest, cchMax, pszFormat, argList);
        // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

        if ((iRet < 0) || (((size_t)iRet) > cchMax))
        {
            // need to null terminate the string
            pszDest += cchMax;
            *pszDest = L'\0';

            // we have truncated pszDest
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
        }
        else if (((size_t)iRet) == cchMax)
        {
            // need to null terminate the string
            pszDest += cchMax;
            *pszDest = L'\0';
        }
    }

    return hr;
}

STRSAFEAPI
KtlStringCbPrintfW(WCHAR* pszDest, size_t cbDest, const WCHAR* pszFormat, ...)
{
    HRESULT hr;
    size_t cchDest;

    cchDest = cbDest / sizeof(WCHAR);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        hr = STRSAFE_E_INVALID_PARAMETER;
    }
    else
    {
        va_list argList;

        va_start(argList, pszFormat);

        hr = KtlStringVPrintfWorkerW(pszDest, cchDest, pszFormat, argList);

        va_end(argList);
    }

    return hr;
}


_Post_satisfies_(cchDest > 0 && cchDest <= cchMax)  
HRESULT KtlStringValidateDestA(  
    _In_reads_opt_(cchDest) STRSAFE_PCNZCH pszDest,  
    _In_ size_t cchDest,  
    _In_ const size_t cchMax)  
{  
    HRESULT hr = S_OK;
    
    if ((cchDest == 0) || (cchDest > cchMax))
    {  
        hr = STRSAFE_E_INVALID_PARAMETER;  
    } 
    return hr;  
}  

STRSAFEWORKERAPI
KtlStringVPrintfWorkerA(
        _Out_writes_(cchDest) _Always_(_Post_z_) STRSAFE_LPSTR pszDest,
        _In_ _In_range_(1, STRSAFE_MAX_CCH) size_t cchDest,
        _Always_(_Out_opt_ _Deref_out_range_(<=, cchDest - 1)) size_t* pcchNewDestLength,
        _In_ _Printf_format_string_ STRSAFE_LPCSTR pszFormat,
        _In_ va_list argList)
{
    HRESULT hr = S_OK;
    int iRet;
    size_t cchMax;
    size_t cchNewDestLength = 0;

    // leave the last space for the null terminator
    cchMax = cchDest - 1;

    iRet = _vsnprintf(pszDest, cchMax, pszFormat, argList);

    if ((iRet < 0) || (((size_t)iRet) > cchMax))
    {
        // need to null terminate the string
        pszDest += cchMax;
        *pszDest = '\0';

        cchNewDestLength = cchMax;

        // we have truncated pszDest
        hr = STRSAFE_E_INSUFFICIENT_BUFFER;
    }
    else if (((size_t)iRet) == cchMax)
    {
        // need to null terminate the string
        pszDest += cchMax;
        *pszDest = '\0';

        cchNewDestLength = cchMax;
    }
    else
    {
        cchNewDestLength = (size_t)iRet;
    }

    if (pcchNewDestLength)
    {
        *pcchNewDestLength = cchNewDestLength;
    }

    return hr;
}

STRSAFEAPI
KtlStringCchPrintfA(
            _Out_writes_(cchDest) _Always_(_Post_z_) STRSAFE_LPSTR pszDest,
            _In_ size_t cchDest,
            _In_ _Printf_format_string_ STRSAFE_LPCSTR pszFormat,
            ...)
{
    HRESULT hr;

    hr = KtlStringValidateDestA(pszDest, cchDest, STRSAFE_MAX_CCH);

    if (SUCCEEDED(hr))
    {
        va_list argList;

        va_start(argList, pszFormat);

        hr = KtlStringVPrintfWorkerA(pszDest,
                cchDest,
                NULL,
                pszFormat,
                argList);

        va_end(argList);
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return hr;
}

STRSAFEAPI
KtlStringCchVPrintfA(
        _Out_writes_(cchDest) _Always_(_Post_z_) STRSAFE_LPSTR pszDest,
        _In_ size_t cchDest,
        _In_ _Printf_format_string_ STRSAFE_LPCSTR pszFormat,
        _In_ va_list argList)
{
    HRESULT hr;

    hr = KtlStringValidateDestA(pszDest, cchDest, STRSAFE_MAX_CCH);

    if (SUCCEEDED(hr))
    {
        hr = KtlStringVPrintfWorkerA(pszDest,
                                  cchDest,
                                  NULL,
                                  pszFormat,
                                  argList);
    }
    return hr;
}

STRSAFEWORKERAPI
KtlStringLengthWorkerA(
        _In_reads_or_z_(cchMax) STRSAFE_PCNZCH psz,
        _In_ _In_range_(<=, STRSAFE_MAX_CCH) size_t cchMax,
        _Out_opt_ _Deref_out_range_(<, cchMax) _Deref_out_range_(<=, _String_length_(psz)) size_t* pcchLength)
{
    HRESULT hr = S_OK;
    size_t cchOriginalMax = cchMax;

    while (cchMax && (*psz != '\0'))
    {
        psz++;
        cchMax--;
    }
    if (cchMax == 0)
    {
        // the string is longer than cchMax
        hr = STRSAFE_E_INVALID_PARAMETER;
    }
    if (pcchLength)
    {
        if (SUCCEEDED(hr))
        {
            *pcchLength = cchOriginalMax - cchMax;
        }
        else
        {
            *pcchLength = 0;
        }
    }
    return hr;
}
STRSAFEAPI
KtlStringCbLengthA(
        _In_reads_or_z_(cbMax) STRSAFE_PCNZCH psz,
        _In_ _In_range_(1, STRSAFE_MAX_CCH * sizeof(char)) size_t cbMax,
        _Out_opt_ _Deref_out_range_(<, cbMax) size_t* pcbLength)
{
    HRESULT hr;
    size_t cchMax = cbMax / sizeof(char);
    size_t cchLength = 0;

    if ((psz == NULL) || (cchMax > STRSAFE_MAX_CCH))
    {
        hr = STRSAFE_E_INVALID_PARAMETER;
    }
    else
    {
        hr = KtlStringLengthWorkerA(psz, cchMax, &cchLength);
    }
    if (pcbLength)
    {
        if (SUCCEEDED(hr))
        {
            // safe to multiply cchLength * sizeof(char) since cchLength < STRSAFE_MAX_CCH and sizeof(char) is 1
            *pcbLength = cchLength * sizeof(char);
        }
        else
        {
            *pcbLength = 0;
        }
    }
    return hr;
}

STRSAFEAPI
KtlStringCbLengthW(
        _In_reads_or_z_(cbMax / sizeof(wchar_t)) STRSAFE_PCNZWCH psz,
        _In_ _In_range_(1, STRSAFE_MAX_CCH * sizeof(wchar_t)) size_t cbMax,
        _Out_opt_ _Deref_out_range_(<, cbMax - 1) size_t* pcbLength)
{
    HRESULT hr;
    size_t cchMax = cbMax / sizeof(wchar_t);
    size_t cchLength = 0;
    if ((psz == NULL) || (cchMax > STRSAFE_MAX_CCH))
    {
        hr = STRSAFE_E_INVALID_PARAMETER;
    }
    else
    {
        hr = KtlStringLengthWorkerW(psz, cchMax, &cchLength);
    }
    if (pcbLength)
    {
        if (SUCCEEDED(hr))
        {
            // safe to multiply cchLength * sizeof(wchar_t) since cchLength < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            *pcbLength = cchLength * sizeof(wchar_t);
        }
        else
        {
            *pcbLength = 0;
        }
    }
    return hr;
}

STRSAFEAPI
KtlStringCchVPrintfW(
        __out_ecount(cchDest) STRSAFE_LPWSTR pszDest,
        __in size_t cchDest,
        __in __format_string STRSAFE_LPCWSTR pszFormat,
        __in va_list argList)
{
    string fmtA = utf16to8(pszFormat);
    vsnprintf((char*)pszDest, cchDest * sizeof(wchar_t), fmtA.c_str(), argList);
    wstring destW = utf8to16((char*)pszDest);
    int len = __min(cchDest - 1, destW.length());
    memcpy(pszDest, destW.c_str(), len * sizeof(wchar_t));
    pszDest[len] = 0;
    return S_OK;
}

