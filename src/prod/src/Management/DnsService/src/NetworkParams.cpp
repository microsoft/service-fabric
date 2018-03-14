// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "NetworkParams.h"

const WCHAR RegistryPath[] = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters";
const WCHAR SearchListName[] = L"SearchList";

void NetworkParams::Create(
    __out NetworkParams::SPtr& spParams,
    __in KAllocator& allocator,
    __in IDnsTracer& tracer
)
{
    spParams = _new(TAG, allocator) NetworkParams(tracer);

    KInvariant(spParams != nullptr);
}

NetworkParams::NetworkParams(
    __in IDnsTracer& tracer
) : _arrDnsServers(GetThisAllocator()),
    _arrDnsSearchList(GetThisAllocator()),
    _tracer(tracer)
{
}

NetworkParams::~NetworkParams()
{
}

void NetworkParams::Initialize()
{
    GetSearchList(/*out*/_arrDnsSearchList, GetThisAllocator(), _tracer);
    GetDnsServerList(/*out*/_arrDnsServers, GetThisAllocator());
}

/*static*/
void NetworkParams::GetDnsServerList(
    __out KArray<ULONG>& arrDnsServers,
    __in KAllocator& allocator
)
{
#if !defined(PLATFORM_UNIX)
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;

    if (0 == WSAStartup(wVersionRequested, &wsaData))
    {
        KBuffer::SPtr spBuffer;

        DWORD flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST;
        IP_ADAPTER_ADDRESSES* pAddresses = NULL;
        ULONG size = 0;
        if (ERROR_BUFFER_OVERFLOW == GetAdaptersAddresses(AF_INET, flags, NULL, pAddresses, &size))
        {
            if (STATUS_SUCCESS != KBuffer::Create(size, /*out*/spBuffer, allocator))
            {
                KInvariant(false);
            }
        }

        pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(spBuffer->GetBuffer());

        if (GetAdaptersAddresses(AF_INET, flags, NULL, pAddresses, &size) != ERROR_SUCCESS)
        {
            return;
        }

        IP_ADAPTER_ADDRESSES* pCurrAddresses = pAddresses;
        while (pCurrAddresses != NULL)
        {
            IP_ADAPTER_DNS_SERVER_ADDRESS* pDnsServer = pCurrAddresses->FirstDnsServerAddress;
            while (pDnsServer != NULL)
            {
                bool found = false;
                SOCKADDR_IN& dns = (SOCKADDR_IN&)*pDnsServer->Address.lpSockaddr;

                IP_ADAPTER_UNICAST_ADDRESS* pUnicast = pCurrAddresses->FirstUnicastAddress;
                while (pUnicast != NULL)
                {
                    SOCKADDR_IN& unicast = (SOCKADDR_IN&)*pUnicast->Address.lpSockaddr;
                    if (dns.sin_addr.s_addr == unicast.sin_addr.s_addr)
                    {
                        found = true;
                        break;
                    }

                    pUnicast = pUnicast->Next;
                }

                if (!found)
                {
                    if (STATUS_SUCCESS != arrDnsServers.Append(dns.sin_addr.s_addr))
                    {
                        KInvariant(false);
                    }
                }
                pDnsServer = pDnsServer->Next;
            }

            pCurrAddresses = pCurrAddresses->Next;
        }

        WSACleanup();
    }
#else
    struct __res_state state;
    int error = res_ninit(/*out*/&state);
    if (error == -1)
    {
        return;
    }

    for (int i = 0; i < state.nscount; i++)
    {
        SOCKADDR_IN& addr = state.nsaddr_list[i];
        if (STATUS_SUCCESS != arrDnsServers.Append(addr.sin_addr.s_addr))
        {
            KInvariant(false);
        }
    }
#endif
}

/*static*/
void NetworkParams::GetSearchList(
    __out KArray<KString::SPtr>& arrSearchList,
    __in KAllocator& allocator,
    __in IDnsTracer& tracer
)
{
#if !defined(PLATFORM_UNIX)
    // Windows stores all DNS information in the registry, API for retrieval of search list does not exist.
    // If a search list is found in the registry, it is used, otherwise the domain name is used.
    // The two are are mutually exclusive.

    tracer.Trace(DnsTraceLevel_Info,
        "NetworkParams GetSearchList, trying to get search list from registry path {0}",
        WSTR(RegistryPath));

    HKEY hKey;
    LSTATUS err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegistryPath, 0, KEY_READ, &hKey);
    if (err == ERROR_SUCCESS)
    {
        DWORD type = REG_EXPAND_SZ;
        DWORD flags = RRF_NOEXPAND | RRF_RT_REG_EXPAND_SZ | RRF_RT_REG_SZ;
        KLocalString<MAX_PATH> strSearchList;
        DWORD bufferSizeInBytes = strSearchList.BufferSizeInChars() * sizeof(WCHAR);
        err = RegGetValue(hKey, NULL, SearchListName, flags, &type,
            static_cast<LPBYTE>(static_cast<PVOID>(strSearchList)), &bufferSizeInBytes);

        // Returned value is a null terminated string. Set the size to returned value - 1 for null terminated char.
        LONG numberOfChars = (bufferSizeInBytes / sizeof(WCHAR)) - 1;
        if (err == ERROR_SUCCESS && numberOfChars > 0)
        {
            strSearchList.SetLength(numberOfChars);

            tracer.Trace(DnsTraceLevel_Info,
                "NetworkParams GetSearchList, successfully got search list from registry {0} {1}, value {2}",
                WSTR(RegistryPath), WSTR(SearchListName), WSTR(strSearchList));

            KStringView str = strSearchList;
            while (!str.IsEmpty())
            {
                ULONG pos = 0;
                if (!str.Find(',', /*out*/pos))
                {
                    pos = str.Length();
                }

                KStringView suffix = str.LeftString(pos);
                suffix = suffix.StripWs(TRUE /*StripLeading*/, TRUE/*StripTrailling*/);
                if (!suffix.IsEmpty())
                {
                    KString::SPtr spSuffixStr;
                    if (STATUS_SUCCESS != KString::Create(/*out*/spSuffixStr, allocator, suffix))
                    {
                        KInvariant(false);
                    }

                    if (STATUS_SUCCESS != arrSearchList.Append(spSuffixStr))
                    {
                        KInvariant(false);
                    }
                }

                // Pos + 1 because we have to remove the delimiter.
                if (str.Length() > (pos + 1))
                {
                    str = str.RightString(str.Length() - (pos + 1));
                    str = str.StripWs(TRUE /*StripLeading*/, TRUE/*StripTrailling*/);
                }
                else
                {
                    str.Clear();
                }
            }
        }
        else
        {
            tracer.Trace(DnsTraceLevel_Info,
                "NetworkParams GetSearchList, failed to get search list from registry {0} {1}, error {2}",
                WSTR(RegistryPath), WSTR(SearchListName), err);
        }

        RegCloseKey(hKey);
    }
    else
    {
        tracer.Trace(DnsTraceLevel_Info,
            "NetworkParams GetSearchList, failed to open registry path {0} error {1}",
            WSTR(RegistryPath), err);
    }

    // If search list is not found, use domain name
    if (arrSearchList.IsEmpty())
    {
        // Primary Suffix comes from domain name + parent domains
        {
            tracer.Trace(DnsTraceLevel_Info,
                "NetworkParams GetSearchList, trying to get search list from domain name");

            KBuffer::SPtr spBuffer;
            ULONG bufferSize = 0;
            FIXED_INFO* pFixedInfo = nullptr;
            err = GetNetworkParams(pFixedInfo, &bufferSize);
            if (err == ERROR_BUFFER_OVERFLOW)
            {
                if (STATUS_SUCCESS != KBuffer::Create(bufferSize, /*out*/spBuffer, allocator))
                {
                    KInvariant(false);
                }
                pFixedInfo = static_cast<FIXED_INFO*>(spBuffer->GetBuffer());
                err = GetNetworkParams(pFixedInfo, &bufferSize);
            }

            if (err == ERROR_SUCCESS)
            {
                KStringViewA str(pFixedInfo->DomainName);
                str = str.StripWs(TRUE /*StripLeading*/, TRUE/*StripTrailling*/);

                if (str.IsEmpty())
                {
                    tracer.Trace(DnsTraceLevel_Info, "NetworkParams GetSearchList, domain name is empty.");
                }
                // Add domain and subdomains to the list.
                // For example, if domain is redmond.corp.microsoft.com then the result should be:
                // 1. redmond.corp.microsoft.com
                // 2. corp.microsoft.com
                // 3. microsoft.com
                // 4. com
                while (!str.IsEmpty())
                {
                    KString::SPtr spSuffixStr;
                    if (STATUS_SUCCESS != KString::Create(/*out*/spSuffixStr, allocator, str.Length() + 1))
                    {
                        KInvariant(false);
                    }

                    if (!spSuffixStr->CopyFromAnsi(str, str.Length()))
                    {
                        KInvariant(false);
                    }
                    spSuffixStr->SetNullTerminator();

                    tracer.Trace(DnsTraceLevel_Info,
                        "NetworkParams GetSearchList, successfully retrieved domain name {0}",
                        WSTR(*spSuffixStr));

                    if (STATUS_SUCCESS != arrSearchList.Append(spSuffixStr))
                    {
                        KInvariant(false);
                    }

                    ULONG pos = 0;
                    if (str.Find('.', /*out*/pos))
                    {
                        // pos + 1 because the dot has to be removed.
                        str = str.RightString(str.Length() - (pos + 1));
                        str = str.StripWs(TRUE /*StripLeading*/, TRUE/*StripTrailling*/);
                    }
                    else
                    {
                        str.Clear();
                    }
                }
            }
            else
            {
                tracer.Trace(DnsTraceLevel_Info, "NetworkParams GetSearchList call failed GetNetworkParams, error {0}", err);
            }
        }

        // Get connection specific suffixes
        {
            KBuffer::SPtr spBuffer;

            DWORD flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_UNICAST;
            IP_ADAPTER_ADDRESSES* pAddresses = NULL;
            ULONG size = 0;
            err = GetAdaptersAddresses(AF_INET, flags, NULL, pAddresses, &size);
            if (ERROR_BUFFER_OVERFLOW == err) 
            {
                if (STATUS_SUCCESS != KBuffer::Create(size, /*out*/spBuffer, allocator))
                {
                    KInvariant(false);
                }
            }

            pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(spBuffer->GetBuffer());
            err = GetAdaptersAddresses(AF_INET, flags, NULL, pAddresses, &size);

            if (ERROR_SUCCESS == err)
            {
                IP_ADAPTER_ADDRESSES* pCurrAddresses = pAddresses;
                while (pCurrAddresses != NULL)
                {
                    KStringView strSuffix(pCurrAddresses->DnsSuffix);
                    strSuffix = strSuffix.StripWs(TRUE /*StripLeading*/, TRUE/*StripTrailling*/);

                    if (strSuffix.IsEmpty())
                    {
                        tracer.Trace(DnsTraceLevel_Info, "NetworkParams GetSearchList, connection-specifix suffix is empty.");
                    }
                    else
                    {
                        tracer.Trace(DnsTraceLevel_Info,
                            "NetworkParams GetSearchList, successfully added connection-specifix suffix {0}",
                            WSTR(strSuffix));

                        KString::SPtr spSuffixStr;
                        if (STATUS_SUCCESS != KString::Create(/*out*/spSuffixStr, allocator, strSuffix))
                        {
                            KInvariant(false);
                        }

                        if (STATUS_SUCCESS != arrSearchList.Append(spSuffixStr))
                        {
                            KInvariant(false);
                        }
                    }

                    pCurrAddresses = pCurrAddresses->Next;
                }
            }
            else
            {
                tracer.Trace(DnsTraceLevel_Info, "NetworkParams GetSearchList call failed GetAdaptersAddresses, error {0}", err);
            }
        }
    }
#else
    struct __res_state state;
    int error = res_ninit(/*out*/&state);
    if (error == -1)
    {
        return;
    }

    for (int i = 0; i < ARRAYSIZE(state.dnsrch); i++)
    {
        if (state.dnsrch[i] == nullptr)
        {
            break;
        }

        KStringViewA str(state.dnsrch[i]);

        KString::SPtr spSuffixStr;
        if (STATUS_SUCCESS != KString::Create(/*out*/spSuffixStr, allocator, str.Length() + 1))
        {
            KInvariant(false);
        }

        if (!spSuffixStr->CopyFromAnsi(str, str.Length()))
        {
            KInvariant(false);
        }
        spSuffixStr->SetNullTerminator();

        if (STATUS_SUCCESS != arrSearchList.Append(spSuffixStr))
        {
            KInvariant(false);
        }

        tracer.Trace(DnsTraceLevel_Info,
            "NetworkParams GetSearchList, successfully added suffix {0}",
            WSTR(*spSuffixStr));
    }
#endif
}
