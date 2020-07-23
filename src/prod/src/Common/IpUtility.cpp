// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if defined(PLATFORM_UNIX)
#include <ifaddrs.h>
#include <resolv.h>
#include <arpa/inet.h>
#endif

using namespace std;
using namespace Common;

ErrorCode IpUtility::GetDnsServers(
    __out std::vector<std::wstring> & list
    )
{
#if !defined(PLATFORM_UNIX)
    ByteBuffer buffer;
    ULONG bufferSize = 0;

    DWORD error = GetNetworkParams((FIXED_INFO*)buffer.data(), /*inout*/&bufferSize);
    if (error == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(bufferSize);
        error = GetNetworkParams((FIXED_INFO*)buffer.data(), /*inout*/&bufferSize);
    }

    if (error != ERROR_SUCCESS)
    {
        return ErrorCode::FromWin32Error(error);
    }

    FIXED_INFO* pFixedInfo = (FIXED_INFO*)buffer.data();
    ASSERT_IFNOT(pFixedInfo != nullptr, "Expected a valid FIXED_INFO buffer");

    IP_ADDR_STRING* pIPAddr = &pFixedInfo->DnsServerList;
    while (pIPAddr != nullptr)
    {
        std::string address(pIPAddr->IpAddress.String);
        std::wstring addressW;
        StringUtility::Utf8ToUtf16(address, addressW);

        list.push_back(addressW);

        pIPAddr = pIPAddr->Next;
    }
#else
    struct __res_state state;
    int error = res_ninit(/*out*/&state);
    if (error == -1)
    {
        return ErrorCode::FromWin32Error(errno);
    }

    for (int i = 0; i < state.nscount; i++)
    {
        SOCKADDR_IN& addr = state.nsaddr_list[i];

        char address[INET_ADDRSTRLEN];
        Invariant(inet_ntop(AF_INET, &addr.sin_addr, address, sizeof(address)));

        std::wstring addressW;
        StringUtility::Utf8ToUtf16(address, addressW);

        list.push_back(addressW);
    }
#endif

    return ErrorCode::Success();
}

ErrorCode IpUtility::GetDnsServersPerAdapter(
    __out std::map<std::wstring, std::vector<std::wstring>> & map
    )
{
#if !defined(PLATFORM_UNIX)
    const DWORD flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_UNICAST;

    ByteBuffer buffer;
    ULONG bufferSize = 0;
    DWORD error = GetAdaptersAddresses(AF_INET, flags, NULL, (IP_ADAPTER_ADDRESSES*)buffer.data(), &bufferSize);
    if (error == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(bufferSize);
        error = GetAdaptersAddresses(AF_INET, flags, NULL, (IP_ADAPTER_ADDRESSES*)buffer.data(), &bufferSize);
    }

    if (error != ERROR_SUCCESS)
    {
        return ErrorCode::FromWin32Error(error);
    }

    IP_ADAPTER_ADDRESSES* pCurrAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    while (pCurrAddresses != NULL)
    {
        std::wstring adapterName = StringUtility::Utf8ToUtf16(pCurrAddresses->AdapterName);
        std::vector<std::wstring> addresses;

        IP_ADAPTER_DNS_SERVER_ADDRESS* pDnsServer = pCurrAddresses->FirstDnsServerAddress;
        while (pDnsServer != NULL)
        {
            SOCKADDR_IN& dns = (SOCKADDR_IN&)*pDnsServer->Address.lpSockaddr;

            WCHAR address[INET_ADDRSTRLEN];
            InetNtop(AF_INET, &dns.sin_addr, address, ARRAYSIZE(address));
            addresses.push_back(address);

            pDnsServer = pDnsServer->Next;
        }

        if (!addresses.empty())
        {
            map.insert(std::make_pair(std::move(adapterName), std::move(addresses)));
        }

        pCurrAddresses = pCurrAddresses->Next;
    }
#endif
    return ErrorCode::Success();
}

ErrorCode IpUtility::GetIpAddressesPerAdapter(
    __out std::map<std::wstring, std::vector<Common::IPPrefix>> & map,
    bool byFriendlyName
    )
{
#if !defined(PLATFORM_UNIX)
    const DWORD flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

    ByteBuffer buffer;
    ULONG bufferSize = 0;
    DWORD error = GetAdaptersAddresses(AF_INET, flags, NULL, (IP_ADAPTER_ADDRESSES*)buffer.data(), &bufferSize);
    if (error == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(bufferSize);
        error = GetAdaptersAddresses(AF_INET, flags, NULL, (IP_ADAPTER_ADDRESSES*)buffer.data(), &bufferSize);
    }

    if (error != ERROR_SUCCESS)
    {
        return ErrorCode::FromWin32Error(error);
    }

    IP_ADAPTER_ADDRESSES* pCurrAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    while (pCurrAddresses != NULL)
    {
        std::wstring adapterName = !byFriendlyName ? StringUtility::Utf8ToUtf16(pCurrAddresses->AdapterName) : pCurrAddresses->FriendlyName;
        std::vector<IPPrefix> addresses;

        IP_ADAPTER_UNICAST_ADDRESS* pUnicast = pCurrAddresses->FirstUnicastAddress;
        while (pUnicast != NULL)
        {
            Endpoint ep(pUnicast->Address);
            IPPrefix ipPrefix(ep, pUnicast->OnLinkPrefixLength);

            addresses.push_back(ipPrefix);

            pUnicast = pUnicast->Next;
        }

        if (!addresses.empty())
        {
            map.insert(std::make_pair(std::move(adapterName), std::move(addresses)));
        }

        pCurrAddresses = pCurrAddresses->Next;
    }
#else
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1)
    {
        return ErrorCode::FromWin32Error(errno);
    }

    ifa = ifaddr;
    while (ifa != nullptr)
    {
        if ((ifa->ifa_addr != nullptr) && (ifa->ifa_addr->sa_family == AF_INET) && (ifa->ifa_netmask != nullptr))
        {
            SOCKADDR_IN& subnetmask = (SOCKADDR_IN&)*ifa->ifa_netmask;
            ULONG mask = subnetmask.sin_addr.s_addr;
            int prefix = 0;
            // Convert subnet mask to prefix
            for (int i = 0; i < 32; i++)
            {
                if (mask & (1 << i))
                {
                    prefix++;
                }
            }

            std::vector<IPPrefix> addresses;

            Endpoint ep(*(ifa->ifa_addr));
            IPPrefix ipPrefix(ep, prefix);
            addresses.push_back(ipPrefix);

            std::wstring adapterName = StringUtility::Utf8ToUtf16(ifa->ifa_name);

            if (!addresses.empty())
            {
                map.insert(std::make_pair(std::move(adapterName), std::move(addresses)));
            }
        }

        ifa = ifa->ifa_next;
    }

    freeifaddrs(ifaddr);
#endif
    return ErrorCode::Success();
}

ErrorCode IpUtility::GetIpAddressOnAdapter(
    __in std::wstring adapterName,
    ADDRESS_FAMILY addressFamily,
    __out std::wstring & ipAddress
   )
{
    std::map<std::wstring, std::vector<IPPrefix>> adapterIpAddressMap;
    auto error = GetIpAddressesPerAdapter(adapterIpAddressMap, true);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto const & ipAdapter : adapterIpAddressMap)
    {
        if (StringUtility::Contains(ipAdapter.first, adapterName))
        {
            for (auto const & ip : ipAdapter.second)
            {
                if ((addressFamily == AF_INET && ip.IsIPV4()) ||
                    (addressFamily == AF_INET6 && ip.IsIPV6()))
                {
                    ipAddress = ip.GetAddress().GetIpString();
                    return ErrorCode::Success();
                }
            }
        }
    }

    return ErrorCode::FromWin32Error(ERROR_NOT_FOUND);
}

ErrorCode IpUtility::GetAdapterAddressOnTheSameNetwork(
    __in std::wstring input,
    __out std::wstring& output
    )
{
    Common::Endpoint inputEp;
    ErrorCode error = Common::Endpoint::TryParse(input, /*out*/inputEp);
    if (!error.IsSuccess())
    {
        return error;
    }

    typedef std::map<std::wstring, std::vector<IPPrefix>> IpMap;

    IpMap map;
    error = GetIpAddressesPerAdapter(map);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (IpMap::iterator it = map.begin(); it != map.end(); ++it)
    {
        const std::vector<IPPrefix> & ipAddresses = it->second;

        for (int i = 0; i < ipAddresses.size(); i++)
        {
            const IPPrefix& ip = ipAddresses[i];
            if (inputEp.EqualPrefix(ip.GetAddress(), ip.GetPrefixLength()))
            {
                output = ip.GetAddress().GetIpString();
                return ErrorCode::Success();
            }
        }
    }

    return ErrorCode::FromWin32Error(ERROR_NOT_FOUND);
}

ErrorCode IpUtility::GetGatewaysPerAdapter(
    __out map<wstring, vector<wstring>> &map
    )
{
#if !defined(PLATFORM_UNIX)
    const DWORD flags = GAA_FLAG_INCLUDE_GATEWAYS;

    ByteBuffer buffer;
    ULONG bufferSize = 0;
    DWORD error = GetAdaptersAddresses(AF_INET, flags, NULL, (IP_ADAPTER_ADDRESSES*)buffer.data(), &bufferSize);
    if (error == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(bufferSize);
        error = GetAdaptersAddresses(AF_INET, flags, NULL, (IP_ADAPTER_ADDRESSES*)buffer.data(), &bufferSize);
    }

    if (error != ERROR_SUCCESS)
    {
        return ErrorCode::FromWin32Error(error);
    }

    IP_ADAPTER_ADDRESSES* pCurrAddress = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    while (pCurrAddress)
    {
        std::wstring adapterName = StringUtility::Utf8ToUtf16(pCurrAddress->AdapterName);
        std::vector<std::wstring> addresses;

        PIP_ADAPTER_GATEWAY_ADDRESS_LH gatewayAddress = pCurrAddress->FirstGatewayAddress;
        while (gatewayAddress)
        {
            SOCKADDR_IN& sockAddr = (SOCKADDR_IN&)*gatewayAddress->Address.lpSockaddr;
            WCHAR ip[16];
            InetNtop(AF_INET, &sockAddr.sin_addr, ip, ARRAYSIZE(ip));
            addresses.push_back(ip);

            gatewayAddress = gatewayAddress->Next;
        }

        if (!addresses.empty())
        {
            map.insert(std::make_pair(std::move(adapterName), std::move(addresses)));
        }

        pCurrAddress = pCurrAddress->Next;
    }

    return ErrorCodeValue::Success;

#else if

    return ErrorCodeValue::NotImplemented;

#endif
}

ErrorCode IpUtility::GetFirstNonLoopbackAddress(
    std::map<std::wstring, std::vector<Common::IPPrefix>> addressesPerAdapter,
    __out wstring & nonLoopbackIp)
{
    std::wregex loopbackaddrIPv4(L"(127.0.0.)(.*)", std::regex::ECMAScript);
    std::wregex loopbackaddrIPv6(L"(0000:0000:0000:0000:0000:0000:0000)(.*)", std::regex::ECMAScript);

    for (map<wstring, vector<Common::IPPrefix>>::const_iterator it = addressesPerAdapter.begin(); it != addressesPerAdapter.end(); ++it)
    {
        for (vector<Common::IPPrefix>::const_iterator iit = it->second.begin(); iit != it->second.end(); ++iit)
        {
            auto address = iit->GetAddress().GetIpString();
            if (!regex_match(address, loopbackaddrIPv4) && !regex_match(address, loopbackaddrIPv6))
            {
                nonLoopbackIp = address;
                return ErrorCodeValue::Success;
            }
        }
    }

    return ErrorCodeValue::NotFound;
}
