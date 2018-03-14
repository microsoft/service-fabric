// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Common;
using namespace std;

StringLiteral const TraceType("EndpointTest");

BOOST_AUTO_TEST_SUITE(EndpointTests)

BOOST_AUTO_TEST_CASE(LoopbackTest)
{
    Endpoint e1(L"127.0.0.1");
    BOOST_REQUIRE(e1.IsLoopback());

    Endpoint e2(L"127.0.1.1");
    BOOST_REQUIRE(e2.IsLoopback());

    Endpoint e3(L"127.255.255.255");
    BOOST_REQUIRE(e3.IsLoopback());

    Endpoint e4(L"128.0.0.1");
    BOOST_REQUIRE(!e4.IsLoopback());

    Endpoint e5(L"1.0.0.127");
    BOOST_REQUIRE(!e5.IsLoopback());

    Endpoint e6(L"1.1.1.1");
    BOOST_REQUIRE(!e6.IsLoopback());

    Endpoint e7(L"10.0.0.1");
    BOOST_REQUIRE(!e7.IsLoopback());

    Endpoint e8(L"0.0.0.0");
    BOOST_REQUIRE(!e8.IsLoopback());
}

BOOST_AUTO_TEST_CASE(SmbNameIpV4)
{
    {
        wstring ip = L"10.0.0.1";
        Endpoint e(L"10.0.0.1");
        wstring smbName = e.AsSmbServerName();
        Trace.WriteInfo(TraceType, "SmbNameIpV4(1): {0}", smbName);
        BOOST_REQUIRE(smbName == wformatString("\\\\{0}", ip));
    }

    {
        wstring ip = L"10.0.0.1";
        Endpoint e(L"10.0.0.1", 1234);
        wstring smbName = e.AsSmbServerName();
        Trace.WriteInfo(TraceType, "SmbNameIpV4(2): {0}", smbName);
        BOOST_REQUIRE(smbName == wformatString("\\\\{0}", ip));
    }
}

BOOST_AUTO_TEST_CASE(SmbNameIpV6)
{
    {
        wstring ip = L"fe80::20d:56ff:fec6:131b";
        Endpoint e(L"fe80::20d:56ff:fec6:131b");
        wstring smbName = e.AsSmbServerName();
        Trace.WriteInfo(TraceType, "SmbNameIpV6(1): {0}", smbName);
        BOOST_REQUIRE(smbName == wformatString("\\\\[{0}]", ip));
    }

    {
        wstring ip = L"fe80::20d:56ff:fec6:131b";
        Endpoint e(L"fe80::20d:56ff:fec6:131b", 1234);
        wstring smbName = e.AsSmbServerName();
        Trace.WriteInfo(TraceType, "SmbNameIpV6(2): {0}", smbName);
        BOOST_REQUIRE(smbName == wformatString("\\\\[{0}]", ip));
    }
}

static void ToStringTestFunc(const wstring & ip, unsigned short port)
{
    Endpoint e(ip, port);
    wstring s = e.ToString();
    Trace.WriteInfo(TraceType, "ConversionTest: ip = {0}, port = {1}, ToString = {2}", ip, port, s);
    if (e.IsIPv4())
    {
        BOOST_REQUIRE(s == wformatString("{0}:{1}", ip, port));
    }
    else
    {
        BOOST_REQUIRE(s == wformatString("[{0}]:{1}", ip, port));
    }
}

BOOST_AUTO_TEST_CASE(ToStringTestIpV4)
{
    ToStringTestFunc(L"10.0.0.1", 1234);
    ToStringTestFunc(L"10.0.0.1", 0);
}

BOOST_AUTO_TEST_CASE(ToStringTestIpV6)
{
    ToStringTestFunc(L"fe80::20d:56ff:fec6:131b", 2345);
    ToStringTestFunc(L"fe80::20d:56ff:fec6:131b", 0);
}

BOOST_AUTO_TEST_CASE(GetPortsInUse)
{
    auto ports = TestPortHelper::GetPortsInUse();
    for(auto const & port : ports)
    {
        Trace.WriteInfo(TraceType, "port 0x{0:x} is in use", port);
    }

    BOOST_REQUIRE(!ports.empty());
}

BOOST_AUTO_TEST_CASE(Comparisons)
{
    Endpoint a(L"10.0.0.1", 1234);
    Endpoint b(L"10.0.0.2", 1234);
    VERIFY_IS_FALSE( a == b );

    Endpoint c(L"10.0.0.1", 1234);
    VERIFY_IS_TRUE( a == c );
    VERIFY_IS_FALSE( b == c );

    Endpoint d(L"10.0.0.1", 999);
    VERIFY_IS_FALSE( d == a );
    VERIFY_IS_TRUE( d != a );

    VERIFY_IS_TRUE( a.EqualPrefix( b, 24 ) );
    VERIFY_IS_TRUE( a.EqualPrefix( b, 30 ) );
    VERIFY_IS_FALSE( a.EqualPrefix( b, 31 ) );
}

BOOST_AUTO_TEST_CASE(IPv6Comparisons)
{
    Endpoint a(L"fe80::20d:56ff:fec6:131b", 1234 );
    Endpoint b(L"fe80::20d:56ff:fec6:131a", 1234 );
    VERIFY_IS_TRUE( a != b );
    VERIFY_IS_TRUE( a.EqualPrefix( b, 126 ) );
    VERIFY_IS_TRUE( a.EqualPrefix( b, 0 ) );
    VERIFY_IS_TRUE( a.EqualPrefix( b, 127 ) );
    VERIFY_IS_FALSE( a.EqualPrefix( b, 128 ) );

    // Differ in the 126th bit
    Endpoint c(L"fe80::20d:56ff:fec6:131f", 1234 );
    VERIFY_IS_FALSE( a.EqualPrefix( c, 126 ) );
    VERIFY_IS_TRUE( a.EqualPrefix( c, 125 ) );

    VERIFY_IS_TRUE( b.LessPrefix( a, 128 ) );
    VERIFY_IS_TRUE( a.LessPrefix( c, 128 ) );
    VERIFY_IS_FALSE( a.LessPrefix( c, 120 ) );
}

BOOST_AUTO_TEST_CASE(ConstConstructors)
{
    const sockaddr_in6 LinkLocal = { AF_INET6,
        0,
        0,
        in6addr_linklocalprefix,
        0 };
    Endpoint e( *reinterpret_cast<sockaddr const *>(&LinkLocal) );
    Trace.WriteNoise(TraceType, "{0}", e);
}

BOOST_AUTO_TEST_CASE(SmokeTest)
{
    Common::Endpoint address(std::wstring(L"FE80::1:2:3:4"));
    Trace.WriteNoise(TraceType, "{0}", address);

    Common::Endpoint address2(L"FE80::1:2:3:4");
    VERIFY_IS_TRUE(address == address2);

    VERIFY_IS_TRUE(address.IsLinkLocal());
    VERIFY_IS_TRUE(address.IsIPV6());
    VERIFY_IS_FALSE(address.IsIPV4());


    Common::Endpoint address5(L"FE80::1:2:3:4:5");
    VERIFY_IS_TRUE(address5.IsLinkLocal());
    VERIFY_IS_FALSE(address5.IsSiteLocal());

    Common::Endpoint address3(std::wstring(L"127.128.129.130"));
    Trace.WriteNoise(TraceType, "{0}", address3);

    Common::Endpoint address4(L"127.128.129.130");
    VERIFY_IS_TRUE(address3 == address4);

    VERIFY_IS_TRUE(address3.IsIPV4());
    VERIFY_IS_FALSE(address3.IsIPV6());
    VERIFY_IS_FALSE(address3.IsLinkLocal());

    Common::Endpoint address6(L"169.254.0.0");
    VERIFY_IS_TRUE(address6.IsLinkLocal());

    Common::Endpoint address7(L"10.11.10.23");
    VERIFY_IS_TRUE(address7.IsSiteLocal());

    Common::Endpoint address8(L"192.168.0.1");
    VERIFY_IS_TRUE(address8.IsSiteLocal());

    Common::Endpoint address9(L"172.32.254.254");
    VERIFY_IS_FALSE(address9.IsSiteLocal());
    Common::Endpoint address10(L"172.31.0.0");
    VERIFY_IS_TRUE(address10.IsSiteLocal());

    Common::Endpoint address11(L"fec0::f090:20b:dbff:fe44:ebda");
    VERIFY_IS_FALSE(address11.IsLinkLocal());
    VERIFY_IS_TRUE(address11.IsSiteLocal());

    Common::Endpoint address12(L"fec0::f70f:0:5efe:157.56.48.15");
    VERIFY_IS_FALSE(address12.IsLinkLocal());
    VERIFY_IS_TRUE(address12.IsSiteLocal());

}

#ifndef PLATFORM_UNIX
BOOST_AUTO_TEST_CASE(SockaddrConstructor)
{
    sockaddr_in6 in6Addr = { AF_INET6,
        htons(3343),
        0,
    { 0xfe,
    0x80,
    0,0,0,0,0,0,
    0x00,
    0xd0,
    0xb7,
    0xff,
    0xfe,
    0x82,
    0x42,
    0xb6 },
    13 };
    Endpoint e( * reinterpret_cast<sockaddr*>( &in6Addr ) );
    VERIFY_IS_TRUE( e.EqualAddress( Endpoint( L"fe80::d0:b7ff:fe82:42b6%13",
        3343 ) ) );
    VERIFY_IS_TRUE( e.Port == 3343 );
    VERIFY_IS_TRUE( e.ScopeId == 13 );
    VERIFY_IS_TRUE( e.IsLinkLocal() );
    VERIFY_IS_TRUE( e.IsIPV6() );
    VERIFY_IS_TRUE( e.AddressLength == sizeof( sockaddr_in6 ) );
};
#endif

BOOST_AUTO_TEST_SUITE_END()

#if 0 // move to IpPrefix.Test.cpp
void TestEndpoints::GeneratePrefixFromAddress()
{
    {
        IPPrefix prefix( Endpoint( L"FE80::1:2:3:4" ), 112 );

        prefix.ZeroNonPrefixBits();

        VERIFY_IS_TRUE( prefix.GetAddress() == Endpoint( L"FE80::1:2:3:0" ) );
    }

    {
        IPPrefix prefix1( Endpoint( L"172.24.1.2" ), 10 );

        VERIFY_IS_FALSE( prefix1.GetAddress() == Endpoint( L"172.0.0.0" ) );

        prefix1.ZeroNonPrefixBits();

        VERIFY_IS_TRUE( prefix1.GetAddress() == Endpoint( L"172.0.0.0" ) );
    }

    {
        IPPrefix prefix2( Endpoint( L"FE80::1:2:3:4" ), 128 );

        prefix2.ZeroNonPrefixBits();

        VERIFY_IS_TRUE( prefix2.GetAddress() == Endpoint( L"fe80::1:2:3:4" ) );
    }

    {
        IPPrefix prefix3( Endpoint( L"FE80::1:2:3:4" ), 0 );

        prefix3.ZeroNonPrefixBits();

        VERIFY_IS_TRUE( prefix3.GetAddress() == Endpoint( L"::" ) );
    }

    {
        IPPrefix prefix4( Endpoint( L"FE80::1:2:3:4" ), 110 );

        prefix4.ZeroNonPrefixBits();

        VERIFY_IS_TRUE( prefix4.GetAddress() == Endpoint( L"fe80::1:2:0:0" ) );
    }
}
#endif
