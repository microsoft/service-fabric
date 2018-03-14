// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TestCommon.h"
#include <iostream>

namespace TransportUnitTest
{
    using namespace Transport;
    using namespace std;
    using namespace Common;

    BOOST_AUTO_TEST_SUITE2(TcpUtilityTests)

    BOOST_AUTO_TEST_CASE(IsLocalEndpoint)
    {
        {
            Endpoint endpoint(L"127.0.0.1");
            Trace.WriteInfo(TraceType, "{0}", endpoint);
            VERIFY_IS_TRUE(TcpTransportUtility::IsLocalEndpoint(endpoint));
        }

        {
            Endpoint endpoint(L"127.0.0.1", 1234);
            Trace.WriteInfo(TraceType, "{0}", endpoint);
            VERIFY_IS_TRUE(TcpTransportUtility::IsLocalEndpoint(endpoint));
        }

        {
            Endpoint endpoint(L"::1");
            Trace.WriteInfo(TraceType, "{0}", endpoint);
            VERIFY_IS_TRUE(TcpTransportUtility::IsLocalEndpoint(endpoint));
        }

        {
            Endpoint endpoint(L"::1", 1234);
            Trace.WriteInfo(TraceType, "{0}", endpoint);
            VERIFY_IS_TRUE(TcpTransportUtility::IsLocalEndpoint(endpoint));
        }

        {
            vector<Endpoint> addressesFromDns;
            auto error = TcpTransportUtility::ResolveToAddresses(L"", ResolveOptions::Unspecified, addressesFromDns);
            VERIFY_IS_TRUE(error.IsSuccess());

            for (auto const & iter : addressesFromDns)
            {
                Trace.WriteInfo(TraceType, "current endpoint: {0}", iter);
                VERIFY_IS_TRUE(TcpTransportUtility::IsLocalEndpoint(iter));
            }
        }

        {
            Endpoint endpoint(L"255.255.255.255");
            Trace.WriteInfo(TraceType, "{0}", endpoint);
            VERIFY_IS_FALSE(TcpTransportUtility::IsLocalEndpoint(endpoint));
        }
    }

    BOOST_AUTO_TEST_CASE(ResolveHostNameAddress)
    {
        // resolve host names
        wstring input = L"localhost:1234";
        vector<Endpoint> output;
        auto retval = TcpTransportUtility::TryResolveHostNameAddress(input, ResolveOptions::Unspecified, output);
        VERIFY_IS_TRUE(retval.IsSuccess());
        for (auto iter = output.cbegin(); iter != output.cend(); ++ iter)
        {
            Trace.WriteInfo(TraceType, "{0} -> {1}", input, *iter);
        }
        VERIFY_IS_FALSE(output.empty());

        wstring thisHost;
        VERIFY_IS_TRUE(TcpTransportUtility::GetLocalFqdn(thisHost).IsSuccess());
        thisHost += L":1234";
        Trace.WriteInfo(TraceType, "input address string with local hostname: {0}", thisHost);
        retval = TcpTransportUtility::TryResolveHostNameAddress(thisHost, ResolveOptions::Unspecified, output);
        VERIFY_IS_TRUE(retval.IsSuccess());
        for (auto iter = output.cbegin(); iter != output.cend(); ++ iter)
        {
            Trace.WriteInfo(TraceType, "{0} -> {1}", thisHost, *iter);
        }
        VERIFY_IS_FALSE(output.empty());

        // negative cases
        input = L"@@@@@@@:1234";
        retval = TcpTransportUtility::TryResolveHostNameAddress(input, ResolveOptions::Unspecified, output);
        VERIFY_IS_FALSE(retval.IsSuccess());

        input = L"localhost:66666";
        retval = TcpTransportUtility::TryResolveHostNameAddress(input, ResolveOptions::Unspecified, output);
        VERIFY_IS_FALSE(retval.IsSuccess());

        input = L"localhost:";
        retval = TcpTransportUtility::TryResolveHostNameAddress(input, ResolveOptions::Unspecified, output);
        VERIFY_IS_FALSE(retval.IsSuccess());

        input = L"localhost";
        retval = TcpTransportUtility::TryResolveHostNameAddress(input, ResolveOptions::Unspecified, output);
        VERIFY_IS_FALSE(retval.IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(ResolveIpAddress)
    {
        wstring name(L"127.0.0.1");

        vector<Endpoint> addresses;
        auto error = TcpTransportUtility::ResolveToAddresses(name, ResolveOptions::Unspecified, addresses);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(addresses.size() == 1);

        error = TcpTransportUtility::ResolveToAddresses(name, ResolveOptions::IPv4, addresses);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(addresses.size() == 1);

        wstring returnedName;
        addresses[0].GetIpString(returnedName);

        VERIFY_ARE_EQUAL2(name, returnedName);

        error = TcpTransportUtility::ResolveToAddresses(name, ResolveOptions::IPv6, addresses);
        VERIFY_IS_FALSE(error.IsSuccess());
        VERIFY_IS_TRUE(addresses.size() == 0);

        name = (L"::1");
        error = TcpTransportUtility::ResolveToAddresses(name, ResolveOptions::Unspecified, addresses);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(addresses.size() == 1);

        error = TcpTransportUtility::ResolveToAddresses(name, ResolveOptions::IPv4, addresses);
        VERIFY_IS_FALSE(error.IsSuccess());
        VERIFY_IS_TRUE(addresses.size() == 0);

        error = TcpTransportUtility::ResolveToAddresses(name, ResolveOptions::IPv6, addresses);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(addresses.size() == 1);

        addresses[0].GetIpString(returnedName);

        VERIFY_ARE_EQUAL2(name, returnedName);
    }

    BOOST_AUTO_TEST_CASE(ResolveLocalHost)
    {
        wstring name(L"localhost");

        // This should return loopback addresses
        vector<Endpoint> addresses;
        auto error = TcpTransportUtility::ResolveToAddresses(name, ResolveOptions::IPv4, addresses);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(addresses.size() == 1);

        wstring returnedName;
        addresses[0].GetIpString(returnedName);

        VERIFY_ARE_EQUAL2(L"127.0.0.1", returnedName);
    }

    BOOST_AUTO_TEST_CASE(ResolveEmptyString)
    {
        wstring name(L"");

        vector<Endpoint> addresses;
        auto error = TcpTransportUtility::ResolveToAddresses(name, ResolveOptions::Unspecified, addresses);

        // Machine should have at least one address (loopback if disconnected)
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(addresses.size() >= 1);
    }

    BOOST_AUTO_TEST_CASE(GetLocalAddresses)
    {
        ENTER;

        vector<Endpoint> localAddresses;
        auto error = TcpTransportUtility::GetLocalAddresses(localAddresses);
        VERIFY_IS_TRUE(error.IsSuccess());

        // Machine should have at least one address
        VERIFY_IS_FALSE(localAddresses.empty());
        Trace.WriteInfo(TraceType, "first local address: {0}, scopeLevel={1}", localAddresses.front(), (uint)localAddresses.front().IpScopeLevel());
        for (uint i = 1; i < localAddresses.size(); ++i)
        {
            Trace.WriteInfo(TraceType, "local address: {0}, scopeLevel={1}", localAddresses[i], (uint)localAddresses[i].IpScopeLevel());
            VERIFY_IS_TRUE(localAddresses[i - 1].IpScopeLevel() >= localAddresses[i].IpScopeLevel());
        }

        error = TcpTransportUtility::GetLocalAddresses(localAddresses, ResolveOptions::IPv4);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_FALSE(localAddresses.empty());
        Trace.WriteInfo(TraceType, "first IPv4 local address: {0}, scopeLevel={1}", localAddresses.front(), (uint)localAddresses.front().IpScopeLevel());
        for (uint i = 1; i < localAddresses.size(); ++i)
        {
            Trace.WriteInfo(TraceType, "local IPv4 address: {0}, scopeLevel={1}", localAddresses[i], (uint)localAddresses[i].IpScopeLevel());
            VERIFY_IS_TRUE(localAddresses[i - 1].IpScopeLevel() >= localAddresses[i].IpScopeLevel());
        }

        error = TcpTransportUtility::GetLocalAddresses(localAddresses, ResolveOptions::IPv6);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_FALSE(localAddresses.empty());
        Trace.WriteInfo(TraceType, "first IPv6 local address: {0}, scopeLevel={1}", localAddresses.front(), (uint)localAddresses.front().IpScopeLevel());
        for (uint i = 1; i < localAddresses.size(); ++i)
        {
            Trace.WriteInfo(TraceType, "local IPv6 address: {0}, scopeLevel={1}", localAddresses[i], (uint)localAddresses[i].IpScopeLevel());
            VERIFY_IS_TRUE(localAddresses[i - 1].IpScopeLevel() >= localAddresses[i].IpScopeLevel());
        }

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TryParseEndpointString)
    {
        Endpoint ep;
        auto err = TcpTransportUtility::TryParseEndpointString(L"127.0.0.1:65535", ep);
        VERIFY_ARE_EQUAL2(err.ReadValue(), ErrorCodeValue::Success);
        err = TcpTransportUtility::TryParseEndpointString(L"0.0.0.0:65535", ep);
        VERIFY_ARE_EQUAL2(err.ReadValue(), ErrorCodeValue::Success);
        err = TcpTransportUtility::TryParseEndpointString(L"127.0.0.1:40913/9edf90e2-4c0c-4900-96aa-12a0e90b3ee1-130898382940996284", ep);
        VERIFY_ARE_EQUAL2(err.ReadValue(), ErrorCodeValue::Success);

        err = TcpTransportUtility::TryParseEndpointString(L"127.0.0.1:-1", ep);
        VERIFY_IS_FALSE(err.IsSuccess());

        err =TcpTransportUtility::TryParseEndpointString(L"127.0.0.1:65536", ep);
        VERIFY_IS_FALSE(err.IsSuccess());

        err = TcpTransportUtility::TryParseEndpointString(L"127.0.0.1::65535", ep);
        VERIFY_IS_FALSE(err.IsSuccess());

        err = TcpTransportUtility::TryParseEndpointString(L"127.0.0.1:", ep);
        VERIFY_IS_FALSE(err.IsSuccess());

        err = TcpTransportUtility::TryParseEndpointString(L"127.0.0.1", ep);
        VERIFY_IS_FALSE(err.IsSuccess());

        wstring thisHost;
        VERIFY_IS_TRUE(TcpTransportUtility::GetLocalFqdn(thisHost).IsSuccess());
        auto addrString = thisHost + L":1234";
        Trace.WriteInfo(TraceType, "input address string with local hostname: {0}", addrString);
        err = TcpTransportUtility::TryParseEndpointString(addrString, ep);
        VERIFY_IS_FALSE(err.IsSuccess());
 
    }

    BOOST_AUTO_TEST_CASE(IsValidEndpointString)
    {
        VERIFY_IS_TRUE(TcpTransportUtility::IsValidEndpointString(L"127.0.0.1:65535"));
        VERIFY_IS_TRUE(TcpTransportUtility::IsValidEndpointString(L"0.0.0.0:65535"));
        VERIFY_IS_TRUE(TcpTransportUtility::IsValidEndpointString(L"127.0.0.1:40913/9edf90e2-4c0c-4900-96aa-12a0e90b3ee1-130898382940996284"));

        VERIFY_IS_FALSE(TcpTransportUtility::IsValidEndpointString(L"127.0.0.1:-1"));
        VERIFY_IS_FALSE(TcpTransportUtility::IsValidEndpointString(L"127.0.0.1:65536"));
        VERIFY_IS_FALSE(TcpTransportUtility::IsValidEndpointString(L"127.0.0.1::65535"));
        VERIFY_IS_FALSE(TcpTransportUtility::IsValidEndpointString(L"127.0.0.1:"));
        VERIFY_IS_FALSE(TcpTransportUtility::IsValidEndpointString(L"127.0.0.1"));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
