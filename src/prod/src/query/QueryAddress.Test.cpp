// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "Query.h"
#include "Common/Common.h"

using namespace std;
using namespace Common;
using namespace Query;

const StringLiteral TraceType("QueryAddressTest");

class QueryAddressTest
{
protected:
    static void TestQueryAddress(
        wstring const & addressString, 
        wstring const & targetedSegment, 
        wstring const & expectedSegmentAddress,
        wstring const & expectedSegmentAddressMetadata,
        ErrorCodeValue::Enum const & expectedError = ErrorCodeValue::Success);
};

BOOST_FIXTURE_TEST_SUITE(QueryAddressTestSuite,QueryAddressTest)

BOOST_AUTO_TEST_CASE(GatewayDestnationAddress)
{
    TestQueryAddress(L"/", L"/", L".", L"");
}

BOOST_AUTO_TEST_CASE(NonGatewayDestinationSegment)
{
    TestQueryAddress(L"/Segment1/Segment2[Metadata]", L"Segment2", L".", L"");
}

BOOST_AUTO_TEST_CASE(AddressWithoutMetadata)
{
    TestQueryAddress(L"/Segment1/Segment2", L"Segment1", L"Segment2", L"");
}

BOOST_AUTO_TEST_CASE(AddressWithMetadata)
{
    TestQueryAddress(L"/Test[Metadata]", L"/", L"Test", L"Metadata");
}

BOOST_AUTO_TEST_CASE(NonForwardableAddress)
{
    TestQueryAddress(L"/Segment1/Segment2", L"Segment3", L"", L"", ErrorCodeValue::InvalidAddress);
}

BOOST_AUTO_TEST_SUITE_END()

void QueryAddressTest::TestQueryAddress(
        wstring const & addressString, 
        wstring const & targetedSegment, 
        wstring const & expectedSegmentAddress,
        wstring const & expectedSegmentAddressMetadata,
        ErrorCodeValue::Enum const &expectedErrorFromSegmentParsing)
{
    QueryAddress address(addressString);
    wstring nextSegment, nextSegmentMetadata;
    auto error = address.GetNextSegmentTo(targetedSegment, nextSegment, nextSegmentMetadata);

    VERIFY_IS_TRUE(
        error.IsError(expectedErrorFromSegmentParsing),
        wformatString("GetNextSegmentTo returned {0}, and the expected error is {1}", error, expectedErrorFromSegmentParsing).c_str());

    if (error.IsSuccess())
    {
        VERIFY_IS_TRUE(
            nextSegment == expectedSegmentAddress, 
            wformatString("Next segment address did not match. Expected = {0}, Found = {1}", expectedSegmentAddress, nextSegment).c_str());

        VERIFY_IS_TRUE(
            nextSegmentMetadata == expectedSegmentAddressMetadata, 
            wformatString("Next segment address metadata did not match. Expected = {0}, Found = {1}", expectedSegmentAddressMetadata, nextSegmentMetadata).c_str());
    }
}
