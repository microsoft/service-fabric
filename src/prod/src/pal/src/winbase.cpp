// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "winbase.h"
#include "util/pal_hosting_util.h"
#include "util/pal_string_util.h"
#include "util/pal_time_util.h"
#include "winnt.h"
#include <execinfo.h>
#include <string>
#include <time.h>
#include <unistd.h>
#include "Common/Common.h"

using namespace std;
using namespace Pal;

bool GetEnvironmentVariable(wstring const & name, wstring& outValue)
{
    int valueLen = 16; // first shot
    outValue.resize(valueLen);
    for(;;)
    {
        int n = ::GetEnvironmentVariableW(
                name.c_str(), &outValue[0], valueLen);
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

void ReplaceAll(std::string &str, std::string from, std::string to)
{
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

NTSYSAPI
WORD
NTAPI
CaptureStackBackTrace(
    __in DWORD FramesToSkip,
    __in DWORD FramesToCapture,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PDWORD BackTraceHash
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

WINADVAPI
BOOL
WINAPI
GetUserNameW (
    __out_ecount_part_opt(*pcbBuffer, *pcbBuffer) LPWSTR lpBuffer,
    __inout LPDWORD pcbBuffer
    )
{
    if (!pcbBuffer)
    {
        return FALSE;
    }

    uid_t uid = geteuid();
    string unameA, udirA;
    if (GetPwUid(uid, unameA, udirA) == 0)
    {
        wstring unameW = utf8to16(unameA.c_str());
        if (*pcbBuffer > unameW.length() && lpBuffer != NULL)
        {
            memcpy_s(lpBuffer, sizeof(wchar_t) * (*pcbBuffer), unameW.c_str(), sizeof(wchar_t)* (unameW.length() + 1));
        }

        *pcbBuffer = unameW.length() + 1;
        return TRUE;
    }

    return FALSE;
}

LONG
InterlockedXor (
    _Inout_ LONG volatile *Destination,
    _In_ LONG Value
    )
{
    return __sync_fetch_and_xor(Destination, Value);
}

LONG64
InterlockedAnd64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    )
{
    return __sync_fetch_and_and(Destination, Value);
}

LONG64
InterlockedOr64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    )
{
    return __sync_fetch_and_or(Destination, Value);
}

LONG64
InterlockedXor64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    )
{
    return __sync_fetch_and_xor(Destination, Value);
}

WINBASEAPI
VOID
WINAPI
RaiseFailFastException(
    _In_opt_ PEXCEPTION_RECORD pExceptionRecord,
    _In_opt_ PCONTEXT pContextRecord,
    _In_ DWORD dwFlags
    )
{
    abort();
}

WINBASEAPI
DWORD
WINAPI
GetProcessId(
    __in HANDLE Process
    )
{
    return (DWORD)Process;
}

WINBASEAPI
BOOL
WINAPI
SystemTimeToFileTime(
    __in  CONST SYSTEMTIME *lpSystemTime,
    __out LPFILETIME lpFileTime
    )
{
    struct tm tm = { 0 };
    time_t posix_time;
    uint64_t win_time;

    if (lpSystemTime->wYear < 1900 ||
        lpSystemTime->wMonth < 1 || lpSystemTime->wMonth > 12 ||
        lpSystemTime->wDay < 1 || lpSystemTime->wDay > 31 ||
        lpSystemTime->wHour >= 24 ||
        lpSystemTime->wMinute >= 60 ||
        lpSystemTime->wSecond >= 60 ||
        lpSystemTime->wMilliseconds >= 1000)
    {
        return FALSE;
    }

    tm.tm_year = lpSystemTime->wYear - 1900;
    tm.tm_mon = lpSystemTime->wMonth - 1;
    tm.tm_mday = lpSystemTime->wDay;
    tm.tm_hour = lpSystemTime->wHour;
    tm.tm_min = lpSystemTime->wMinute;
    tm.tm_sec = lpSystemTime->wSecond;

    posix_time = timegm(&tm);
    if (posix_time == -1)
    {
        return FALSE;
    }
    win_time = posix_time;

    // Convert to 100ns unit
    win_time *= 10000000;
    win_time += WINDOWS_LINUX_EPOCH_BIAS;
    win_time += uint32_t(lpSystemTime->wMilliseconds) * 10000;

    lpFileTime->dwLowDateTime = (uint32_t)(win_time);
    lpFileTime->dwHighDateTime = (uint32_t)(win_time >> 32);

    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
FileTimeToLocalFileTime(
    __in  CONST FILETIME *lpFileTime,
    __out LPFILETIME lpLocalFileTime
    )
{
    const __int64 SECS_BETWEEN_1601_AND_1970_EPOCHS = 11644473600LL;
    const __int64 SECS_TO_100NS = 10000000; /* 10^7 */

    __int64 t = *((__int64*)lpFileTime) - (SECS_BETWEEN_1601_AND_1970_EPOCHS * SECS_TO_100NS);
    __int64 remain = t % SECS_TO_100NS;
    t /= SECS_TO_100NS; /* now convert to seconds */

    struct tm local;
    localtime_r(static_cast<time_t const*>(&t), &local);
    t = timegm(&local);

    *((__int64*)lpLocalFileTime) = (t + SECS_BETWEEN_1601_AND_1970_EPOCHS) * SECS_TO_100NS + remain;

    return TRUE;
}

WINBASEAPI
DWORD
WINAPI
FormatMessageA(
    __in     DWORD dwFlags,
    __in_opt LPCVOID lpSource,
    __in     DWORD dwMessageId,
    __in     DWORD dwLanguageId,
    __out    LPSTR lpBuffer,
    __in     DWORD nSize,
    __in_opt va_list *Arguments
    )
{
    // TODO: Change the caller: WinErrorCategory.cpp
    return 0;
}

WINBASEAPI
__success(return <= nSize)
__success(return != 0)
DWORD
WINAPI
ExpandEnvironmentStringsW(
    __in LPCWSTR lpSrc,
    __out_ecount_part_opt(nSize, return) LPWSTR lpDst,
    __in DWORD nSize
    )
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
        memcpy(lpDst, dest.c_str(), sizeof(wchar_t)*dest.length());
        lpDst[dest.length()] = 0;
    }
    return dest.length() + 1;
}

WINADVAPI
BOOL
WINAPI
CreateWellKnownSid(
    __in     WELL_KNOWN_SID_TYPE WellKnownSidType,
    __in_opt PSID DomainSid,
    __out_bcount_part_opt(*cbSid, *cbSid) PSID pSid,
    __inout  DWORD *cbSid
    )
{
    string serviceFabricUserName = "sfuser";

    if(*cbSid < sizeof(LSID))
    {
        return FALSE;
    }
    if(WellKnownSidType==WinNetworkServiceSid)
    {
        // On linux Fabric is run as sfuser instead of network service. This here replaces
        // the user with the sfuser id when network service is requested
        PLSID psid = (PLSID)pSid;
        psid->Revision = SID_REVISION;
        psid->SubAuthorityCount = 2;
        psid->IdentifierAuthority =SECURITY_LOCAL_SID_AUTHORITY;
        psid->SubAuthority[0] = SECURITY_LINUX_DOMAIN_RID;
        do
        {
            errno = 0;
            uid_t uid;
            if(0 == GetPwUname(serviceFabricUserName, &uid))
            {
                psid->SubAuthority[1] = uid;
                return TRUE;
            }
            else if (errno == EINTR || errno == EAGAIN)
            {
                continue;
            }
            else
            {
                return FALSE;
            }
        } while(true);
    }

    if(WellKnownSidType==WinLocalSystemSid)
    {
        PLSID psid = (PLSID)pSid;
        psid->Revision = SID_REVISION;
        psid->SubAuthorityCount = 2;
        psid->IdentifierAuthority =SECURITY_LOCAL_SID_AUTHORITY;
        psid->SubAuthority[0] = SECURITY_LINUX_DOMAIN_RID;
        psid->SubAuthority[1] = 0;
        return TRUE;
    }

    if(WellKnownSidType==WinAnonymousSid || WellKnownSidType==WinWorldSid)
    {
        PLSID psid = (PLSID)pSid;
        psid->Revision = SID_REVISION;
        psid->SubAuthorityCount = 2;
        psid->IdentifierAuthority =SECURITY_LOCAL_SID_AUTHORITY;
        psid->SubAuthority[0] = SECURITY_LINUX_DOMAIN_RID;
        do
        {
            errno = 0;
            uid_t uid;
            if (0 == GetPwUname(serviceFabricUserName, &uid, 0))
            {
                psid->SubAuthority[1] = uid; //TODO: Linux does not have anonymous...
                return TRUE;
            }
            else if (errno == EINTR || errno == EAGAIN)
            {
                continue;
            }
            else
            {
                return FALSE;
            }
        } while (true);
    }

    if(WellKnownSidType==WinBuiltinAdministratorsSid)
    {
        PLSID psid = (PLSID)pSid;
        psid->Revision = SID_REVISION;
        psid->SubAuthorityCount = 2;
        psid->IdentifierAuthority =SECURITY_LOCAL_SID_AUTHORITY;
        psid->SubAuthority[0] = SECURITY_LINUX_DOMAIN_GROUP_RID;
        psid->SubAuthority[1] = 0;
        return TRUE;
    }

    // unimplemented except above SidType(s)
    Common::Assert::CodingError("{0}: unsupported sid type: {1}",  __func__, (int)WellKnownSidType);
    return FALSE;
}


WINBASEAPI
__success(return != 0)
BOOL
WINAPI
GetComputerNameExW (
    __in    COMPUTER_NAME_FORMAT NameType,
    __out_ecount_part_opt(*nSize, *nSize + 1) LPWSTR lpBuffer,
    __inout LPDWORD nSize
    )
{
    memcpy(lpBuffer, L"Test", 10);
    return TRUE;
}

BOOLEAN
APIENTRY
CreateSymbolicLinkW (
    __in LPCWSTR lpSymlinkFileName,
    __in LPCWSTR lpTargetFileName,
    __in DWORD dwFlags
    )
{
    UNREFERENCED_PARAMETER(dwFlags);
    string symNameA = utf16to8(lpSymlinkFileName);
    string tgtNameA = utf16to8(lpTargetFileName);

    ReplaceAll(symNameA, "\\", "/");
    ReplaceAll(tgtNameA, "\\", "/");
    int ret = symlink(tgtNameA.c_str(), symNameA.c_str());
    return ret == 0;
}
