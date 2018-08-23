// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "LeaseLayerTestCommon.h"

#include <stdio.h>
#include <string.h>

namespace LeaseLayerTestCommon
{
   void PrintLeaseTaefTestSummary(LONG passCount, LONG failCount)
    {
        Trace.WriteInfo(TraceType, "Pass Count: {0}", passCount);
        Trace.WriteInfo(TraceType, "Fail Count: {0}", failCount);
        
        if (failCount != 0)
        {
            Trace.WriteInfo(TraceType, "[FAIL]");
        }
        else
        {
            Trace.WriteInfo(TraceType, "[PASS]");          
        }
    }
   
    bool GetAddressesHelper(
       TRANSPORT_LISTEN_ENDPOINT & socketAddress1, 
       TRANSPORT_LISTEN_ENDPOINT & socketAddress2, 
       TRANSPORT_LISTEN_ENDPOINT & socketAddress6_1, 
       TRANSPORT_LISTEN_ENDPOINT & socketAddress6_2,
       bool & foundV4,
       bool & foundV6
       )    
    {
        WCHAR LocalComputerName[ENDPOINT_ADDR_CCH_MAX] = {0};
        DWORD dwSize = ENDPOINT_ADDR_CCH_MAX;
        GetComputerNameEx(
            ComputerNameDnsFullyQualified,
            (LPWSTR)&LocalComputerName, &dwSize);

        // Setup hints
        ADDRINFOW hints = {};
        hints.ai_flags    = AI_ADDRCONFIG;
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        ADDRINFOW *result = nullptr;
        ADDRINFOW *ptr = nullptr;
#if defined(PLATFORM_UNIX)
        DWORD dwRetval = GetAddrInfoW(L"", nullptr, &hints, &result);
#else
        DWORD dwRetval = GetAddrInfoW(LocalComputerName, nullptr, &hints, &result);
#endif
        if (dwRetval != 0) 
        {
            Trace.WriteInfo(TraceType, "GetAddrInfoW failed with error: {0}\n", dwRetval);
            return false; 
        }

        if (result == nullptr)
        {
            Trace.WriteInfo(TraceType, "GetAddrInfoW succeeded but result is null");
            return false;
        }

        const u_short port = 20017;
        for (ptr = result; (ptr != nullptr) && !(foundV4 && foundV6); ptr = ptr->ai_next) 
        {
            if (ptr->ai_family == AF_INET)
            {
                if (foundV4)
                {
                    Trace.WriteInfo(TraceType, "Already found v4, skipping");
                    continue;
                }

                Trace.WriteInfo(TraceType, "AF_INET (IPv4)");
                socketAddress1.ResolveType = (ADDRESS_FAMILY)ptr->ai_family;
                socketAddress1.Port = port + 1;

#if defined(PLATFORM_UNIX)
                ::sockaddr sockaddr = *(ptr->ai_addr);
                Common::Endpoint endpoint(sockaddr);
                wcsncpy(socketAddress1.Address, endpoint.GetIpString2().c_str(), ENDPOINT_ADDR_CCH_MAX);
                socketAddress1.Address[ENDPOINT_ADDR_CCH_MAX - 1] = '\0';
#else
#pragma warning(suppress : 24007) // Dealing with IPv4 specifically
                RtlIpv4AddressToStringW(&((PSOCKADDR_IN)ptr->ai_addr)->sin_addr, socketAddress1.Address);
#endif

                Trace.WriteInfo(TraceType, "\tIPv4 address {0}\n", socketAddress1.Address);

                socketAddress2 = socketAddress1;
                socketAddress2.Port = port + 2;

                foundV4 = true;
            }
            else if (ptr->ai_family == AF_INET6)
            {
                if(foundV6) 
                {
                    Trace.WriteInfo(TraceType, "Already found v6, skipping");
                    continue;
                }

                Trace.WriteInfo(TraceType, "AF_INET (IPv6)");
                socketAddress6_1.ResolveType = (ADDRESS_FAMILY)ptr->ai_family;
                socketAddress6_1.Port = port + 1;

#if defined(PLATFORM_UNIX)
                ::sockaddr sockaddr = *(ptr->ai_addr);
                Common::Endpoint endpoint(sockaddr);
                wcsncpy(socketAddress6_1.Address, endpoint.GetIpString2().c_str(), ENDPOINT_ADDR_CCH_MAX);
                socketAddress6_1.Address[ENDPOINT_ADDR_CCH_MAX - 1] = '\0';
#else
                RtlIpv6AddressToStringW(&((PSOCKADDR_IN6)ptr->ai_addr)->sin6_addr, socketAddress6_1.Address);
#endif

                Trace.WriteInfo(TraceType, "\tIPv6 address {0}\n", socketAddress6_1.Address);

                socketAddress6_2 = socketAddress6_1;
                socketAddress6_2.Port = port + 2;

                foundV6 = true;
            }
        }
       
        Trace.WriteInfo(TraceType, "DEBUG foundv4:{0}", foundV4);
        Trace.WriteInfo(TraceType, "DEBUG foundv6:{0}", foundV6);

        FreeAddrInfoW(result);
        return foundV4 || foundV6;
    }

}
