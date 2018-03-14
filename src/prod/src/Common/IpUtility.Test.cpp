// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Common;
using namespace std;

Common::StringLiteral const TraceType("IpUtilityTest");

BOOST_AUTO_TEST_SUITE(IpUtilityTest)

BOOST_AUTO_TEST_CASE(GetDnsServersTest)
{
    std::vector<std::wstring> dnsServers;
    ErrorCode error = IpUtility::GetDnsServers(/*out*/dnsServers);
    VERIFY_IS_TRUE(error.IsSuccess());

    Trace.WriteInfo(TraceType, "GetDnsServers {0}", dnsServers);

    for (int i = 0; i < dnsServers.size(); i++)
    {
        const std::wstring & address = dnsServers[i];

        Common::Endpoint ep(address);

        // Validate this is ipv4 address
        VERIFY_IS_TRUE(ep.IsIPV4());
    }
}

BOOST_AUTO_TEST_CASE(GetDnsServersPerAdapterTest)
{
#if !defined(PLATFORM_UNIX)
    typedef std::map<std::wstring, std::vector<std::wstring>> IpMap;

    IpMap map;
    ErrorCode error = IpUtility::GetDnsServersPerAdapter(/*out*/map);
    VERIFY_IS_TRUE(error.IsSuccess());

    Trace.WriteInfo(TraceType, "GetDnsServersPerAdapter {0}", map);

    for (IpMap::iterator it = map.begin(); it != map.end(); ++it)
    {
        const std::wstring & adapterName = it->first;
        const std::vector<std::wstring> & dnsAddresses = it->second;

        VERIFY_IS_TRUE(!adapterName.empty());
        VERIFY_IS_TRUE(!dnsAddresses.empty());

        for (int i = 0; i < dnsAddresses.size(); i++)
        {
            const std::wstring & address = dnsAddresses[i];

            Common::Endpoint ep(address);
            // Validate this is ipv4 address
            VERIFY_IS_TRUE(ep.IsIPV4());
        }
    }
#endif
}

BOOST_AUTO_TEST_CASE(GetIpAddressesPerAdapterTest)
{
    typedef std::map<std::wstring, std::vector<Common::IPPrefix>> IpMap;

    IpMap map;
    ErrorCode error = IpUtility::GetIpAddressesPerAdapter(/*out*/map);
    VERIFY_IS_TRUE(error.IsSuccess());

    Trace.WriteInfo(TraceType, "GetIpAddressesPerAdapter {0}", map);

    for (IpMap::iterator it = map.begin(); it != map.end(); ++it)
    {
        const std::wstring & adapterName = it->first;
        const std::vector<Common::IPPrefix> & ipAddresses = it->second;

        VERIFY_IS_TRUE(!adapterName.empty());
        VERIFY_IS_TRUE(!ipAddresses.empty());

        for (int i = 0; i < ipAddresses.size(); i++)
        {
            const Common::IPPrefix & prefix = ipAddresses[i];

            VERIFY_IS_TRUE(prefix.IsIPV4());
        }
    }
}

BOOST_AUTO_TEST_CASE(GetAdapterAddressOnTheSameNetworkTest)
{
    typedef std::map<std::wstring, std::vector<Common::IPPrefix>> IpMap;

    IpMap map;
    IpUtility::GetIpAddressesPerAdapter(/*out*/map);
    for (IpMap::iterator it = map.begin(); it != map.end(); ++it)
    {
        const std::wstring & adapterName = it->first;
        const std::vector<Common::IPPrefix> & ipAddresses = it->second;
        for (int i = 0; i < ipAddresses.size(); i++)
        {
            std::wstring outputIp;
            const Common::IPPrefix & prefix = ipAddresses[i];
            ErrorCode error = IpUtility::GetAdapterAddressOnTheSameNetwork(prefix.GetAddress().GetIpString(), /*out*/outputIp);

            Trace.WriteInfo(TraceType,
                "GetAdapterAddressOnTheSameNetwork adapter {0} IPPrefix {1} => Output {2}, error {3}",
                adapterName, prefix, outputIp, error);

            VERIFY_IS_TRUE(error.IsSuccess());
        }

    }
}

BOOST_AUTO_TEST_SUITE_END()
