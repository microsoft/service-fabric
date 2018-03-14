// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsNodeCacheMonitor.h"

#if !defined(PLATFORM_UNIX)
#include "windns.h"
const WCHAR HostFileRelativePath[] = L"\\drivers\\etc\\hosts";
#endif


/*static*/
void DnsNodeCacheMonitor::Create(
    __out DnsNodeCacheMonitor::SPtr& spCache,
    __in KAllocator& allocator,
    __in const DnsServiceParams& params,
    __in IDnsTracer& tracer,
    __in IDnsCache& dnsCache
)
{
    spCache = _new(TAG, allocator) DnsNodeCacheMonitor(params, tracer, dnsCache);
    KInvariant(spCache != nullptr);
}

DnsNodeCacheMonitor::DnsNodeCacheMonitor(
    __in const DnsServiceParams& params,
    __in IDnsTracer& tracer,
    __in IDnsCache& dnsCache
) : _params(params),
_tracer(tracer),
_cache(dnsCache)
{
}

DnsNodeCacheMonitor::~DnsNodeCacheMonitor()
{
}

bool DnsNodeCacheMonitor::StartMonitor(
    __in_opt KAsyncContextBase* const parent
)
{
#if !defined(PLATFORM_UNIX)    
    ULONG length = GetSystemDirectory(static_cast<LPWSTR>(_strHostsFilePath), _strHostsFilePath.BufferSizeInChars());
    if (0 == length)
    {
        _tracer.Trace(DnsTraceLevel_Warning, "DnsNodeCacheMonitor failed to get path of the system directory");
        return false;
    }
    _strHostsFilePath.SetLength(length);

    if (!_strHostsFilePath.Concat(HostFileRelativePath))
    {
        _tracer.Trace(DnsTraceLevel_Warning, "DnsNodeCacheMonitor failed to retrieve Hosts file path");
        return false;
    }
    _strHostsFilePath.SetNullTerminator();
#endif

    Start(parent, nullptr/*callback*/);
    return true;
}

void DnsNodeCacheMonitor::OnStart()
{
#if !defined(PLATFORM_UNIX)
    FlushCache();

    _cache.RegisterNotification(*this);
#endif    
}

void DnsNodeCacheMonitor::OnCancel()
{
    _tracer.Trace(DnsTraceLevel_Info, "DnsNodeCacheMonitor OnCancel");

#if !defined(PLATFORM_UNIX) 
    _cache.UnregisterNotification(*this);
#endif       

    Complete(STATUS_CANCELLED);
}

bool DnsNodeCacheMonitor::FlushCache()
{
#if !defined(PLATFORM_UNIX)
    // Change to hosts file flushes the node cache and app cache.
    SYSTEMTIME systemTime;
    GetSystemTime(&systemTime);

    FILETIME fileTime;
    SystemTimeToFileTime(&systemTime, &fileTime);

    HANDLE handle = CreateFile(static_cast<LPCWSTR>(_strHostsFilePath),
        FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
    {
        _tracer.Trace(DnsTraceLevel_Warning,
            "DnsNodeCacheMonitor failed to get the file handle {0}, error {1}",
            WSTR(_strHostsFilePath), GetLastError());
        return false;
    }

    if (!SetFileTime(handle, nullptr, nullptr, &fileTime))
    {
        _tracer.Trace(DnsTraceLevel_Warning,
            "DnsNodeCacheMonitor failed to set the file last modified time {0}, error {1}",
            WSTR(_strHostsFilePath), GetLastError());
        return false;
    }
#endif

    return true;
}

void DnsNodeCacheMonitor::OnDnsCachePut(
    __in KString& dnsName
)
{
#if !defined(PLATFORM_UNIX)
    DWORD dwOptions = 0x18010;
    DNS_STATUS status = DnsQuery_W(static_cast<LPCWSTR>(dnsName), DNS_TYPE_A, dwOptions, NULL, NULL, NULL);

    if (status == ERROR_SUCCESS)
    {
        _tracer.Trace(DnsTraceLevel_Info,
            "DnsNodeCacheMonitor OnDnsCachePut flushed entry {0} from node DNS cache.", WSTR(dnsName));

        FlushCache();
    }
#endif
}
