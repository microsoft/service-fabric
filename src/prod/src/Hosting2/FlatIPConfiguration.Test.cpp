// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;

const StringLiteral TraceType("FlatIPConfigurationTest");

class FlatIPConfigurationTest
{
protected:
    FlatIPConfigurationTest() { BOOST_REQUIRE(Setup()); }
    TEST_CLASS_SETUP(Setup)
    ~FlatIPConfigurationTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup)
};

BOOST_FIXTURE_TEST_SUITE(FlatIPConfigurationTestSuite, FlatIPConfigurationTest)

BOOST_AUTO_TEST_CASE(InitialLoadTest)
{
    static const wstring testData = 
        L"<Interfaces><Interface MacAddress=\"000D3A73F074\" IsPrimary=\"true\">"
            L"<IPSubnet Prefix=\"10.0.0.0 / 24\">"
                L"<IPAddress Address=\"10.0.0.4\" IsPrimary=\"true\"/>"
                L"<IPAddress Address=\"10.0.0.5\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.6\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.7\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.8\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.9\" IsPrimary=\"false\"/>"
            L"</IPSubnet>"
        L"</Interface></Interfaces>";

    FlatIPConfiguration config(testData);

    VERIFY_ARE_EQUAL(config.Interfaces.size(), 1);

    auto itf = config.Interfaces.front();
    VERIFY_ARE_EQUAL(itf->MacAddress, 0x000D3A73F074ULL);
    VERIFY_IS_TRUE(itf->Primary);
    VERIFY_ARE_EQUAL(itf->Subnets.size(), 1);

    auto subnet = itf->Subnets.front();
    VERIFY_ARE_EQUAL(IPHelper::Format(subnet->Primary), L"10.0.0.4");
    VERIFY_ARE_EQUAL(IPHelper::Format(subnet->GatewayIp), L"10.0.0.1");
    VERIFY_ARE_EQUAL(subnet->Mask, 0xffffff00);
    VERIFY_ARE_EQUAL(subnet->SecondaryAddresses.size(), 5);
}

BOOST_AUTO_TEST_CASE(DuplicateIPAddressTest)
{
    static const wstring testData = 
        L"<Interfaces><Interface MacAddress=\"000D3A73F074\" IsPrimary=\"true\">"
            L"<IPSubnet Prefix=\"10.0.0.0 / 24\">"
                L"<IPAddress Address=\"10.0.0.4\" IsPrimary=\"true\"/>"
                L"<IPAddress Address=\"10.0.0.5\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.6\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.7\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.8\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.6\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.9\" IsPrimary=\"false\"/>"
            L"</IPSubnet>"
        L"</Interface></Interfaces>";

    static const wstring testData2 = 
        L"<Interfaces><Interface MacAddress=\"000D3A73F074\" IsPrimary=\"true\">"
            L"<IPSubnet Prefix=\"10.0.0.0 / 24\">"
                L"<IPAddress Address=\"10.0.0.4\" IsPrimary=\"true\"/>"
                L"<IPAddress Address=\"10.0.0.5\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.6\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.4\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.7\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.8\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.9\" IsPrimary=\"false\"/>"
            L"</IPSubnet>"
        L"</Interface></Interfaces>";

    static const wstring testData3 = 
        L"<Interfaces><Interface MacAddress=\"000D3A73F074\" IsPrimary=\"true\">"
            L"<IPSubnet Prefix=\"10.0.0.0 / 24\">"
                L"<IPAddress Address=\"10.0.0.4\" IsPrimary=\"true\"/>"
                L"<IPAddress Address=\"10.0.0.5\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.6\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.7\" IsPrimary=\"true\"/>"
                L"<IPAddress Address=\"10.0.0.8\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.9\" IsPrimary=\"false\"/>"
            L"</IPSubnet>"
        L"</Interface></Interfaces>";

    BOOST_CHECK_THROW(FlatIPConfiguration config(testData), XmlException);
    BOOST_CHECK_THROW(FlatIPConfiguration config(testData2), XmlException);
    BOOST_CHECK_THROW(FlatIPConfiguration config(testData3), XmlException);
}

BOOST_AUTO_TEST_CASE(DuplicateSubnetTest)
{
    static const wstring testData = 
        L"<Interfaces><Interface MacAddress=\"000D3A73F074\" IsPrimary=\"true\">"
            L"<IPSubnet Prefix=\"10.0.0.0 / 24\">"
                L"<IPAddress Address=\"10.0.0.4\" IsPrimary=\"true\"/>"
                L"<IPAddress Address=\"10.0.0.5\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.6\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.7\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.8\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.9\" IsPrimary=\"false\"/>"
            L"</IPSubnet>,"
            L"<IPSubnet Prefix=\"10.0.0.0 / 8\">"
                L"<IPAddress Address=\"10.0.0.10\" IsPrimary=\"true\"/>"
            L"</IPSubnet>"
        L"</Interface></Interfaces>";

    BOOST_CHECK_THROW(FlatIPConfiguration config(testData), XmlException);
}

BOOST_AUTO_TEST_CASE(InputToleranceTest)
{
    static const wstring testData = 
        L"<Interfaces><Interface MacAddress=\"000D3A73F074\" IsPrimary=\"true\" SomeOtherAttribute=\"SomeValue\">"
            L"<IPSubnet Prefix=\"10.0.0.0 / 24\" SomeOtherAttribute=\"SomeValue\">"
                L"<IPAddress Address=\"10.0.0.4\" IsPrimary=\"true\" SomeOtherAttribute=\"SomeValue\"/>"
                L"<IPAddress Address=\"10.0.0.5\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.6\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.7\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.8\" IsPrimary=\"false\"/>"
                L"<IPAddress Address=\"10.0.0.9\" IsPrimary=\"false\"><SomeInternalTag /></IPAddress>"
            L"</IPSubnet>"
            L"<SomeOtherTag> </SomeOtherTag>"
        L"</Interface></Interfaces>";

    FlatIPConfiguration config(testData);

    VERIFY_ARE_EQUAL(config.Interfaces.size(), 1);

    auto itf = config.Interfaces.front();
    VERIFY_ARE_EQUAL(itf->MacAddress, 0x000D3A73F074ULL);
    VERIFY_IS_TRUE(itf->Primary);
    VERIFY_ARE_EQUAL(itf->Subnets.size(), 1);

    auto subnet = itf->Subnets.front();
    VERIFY_ARE_EQUAL(IPHelper::Format(subnet->Primary), L"10.0.0.4");
    VERIFY_ARE_EQUAL(IPHelper::Format(subnet->GatewayIp), L"10.0.0.1");
    VERIFY_ARE_EQUAL(subnet->Mask, 0xffffff00);
    VERIFY_ARE_EQUAL(subnet->SecondaryAddresses.size(), 5);
}

BOOST_AUTO_TEST_CASE(CIDRParseTest)
{
    uint baseIp;
    int maskNum;

    VERIFY_IS_TRUE(IPHelper::TryParseCIDR(L"10.0.0.0/24", baseIp, maskNum));
    VERIFY_ARE_EQUAL(24, maskNum);
    VERIFY_ARE_EQUAL(L"10.0.0.0", IPHelper::Format(baseIp));
    VERIFY_ARE_EQUAL(L"10.0.0.0 / 24", IPHelper::FormatCIDR(baseIp, maskNum));

    VERIFY_IS_TRUE(IPHelper::TryParseCIDR(L"10.0.0.0 /24", baseIp, maskNum));
    VERIFY_ARE_EQUAL(24, maskNum);
    VERIFY_ARE_EQUAL(L"10.0.0.0", IPHelper::Format(baseIp));
    VERIFY_ARE_EQUAL(L"10.0.0.0 / 24", IPHelper::FormatCIDR(baseIp, maskNum));

    VERIFY_IS_TRUE(IPHelper::TryParseCIDR(L"10.0.0.0/ 24", baseIp, maskNum));
    VERIFY_ARE_EQUAL(24, maskNum);
    VERIFY_ARE_EQUAL(L"10.0.0.0", IPHelper::Format(baseIp));
    VERIFY_ARE_EQUAL(L"10.0.0.0 / 24", IPHelper::FormatCIDR(baseIp, maskNum));

    VERIFY_IS_TRUE(IPHelper::TryParseCIDR(L"10.0.0.0 / 24", baseIp, maskNum));
    VERIFY_ARE_EQUAL(24, maskNum);
    VERIFY_ARE_EQUAL(L"10.0.0.0", IPHelper::Format(baseIp));
    VERIFY_ARE_EQUAL(L"10.0.0.0 / 24", IPHelper::FormatCIDR(baseIp, maskNum));

    VERIFY_IS_TRUE(IPHelper::TryParseCIDR(L"2.3.4.5 / 24", baseIp, maskNum));
    VERIFY_ARE_EQUAL(24, maskNum);
    VERIFY_ARE_EQUAL(L"2.3.4.5", IPHelper::Format(baseIp));
    VERIFY_ARE_EQUAL(L"2.3.4.5 / 24", IPHelper::FormatCIDR(baseIp, maskNum));
}

BOOST_AUTO_TEST_SUITE_END()

bool FlatIPConfigurationTest::Setup()
{
    return true;
}

bool FlatIPConfigurationTest::Cleanup()
{
    return true;
}
