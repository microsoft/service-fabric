// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;

namespace Common
{
    StringLiteral TraceComponent = "StringResourceTest";

    class StringResourceTest
    {
    protected:
        StringResourceTest() { BOOST_REQUIRE(TestReset()); }
        ~StringResourceTest() { BOOST_REQUIRE(TestReset()); }

        TEST_METHOD_SETUP(TestReset);
    };

    // *****************************************************
    // Example implementation of a string resource component
    // *****************************************************

    // Begin .h
    //

#define TEST_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( TestResource, ResourceProperty, COMMON, ResourceName ) \

    class TestResource
    {
        DECLARE_SINGLETON_RESOURCE( TestResource )

        TEST_RESOURCE( Reserved1, RESERVED1 )
        TEST_RESOURCE( Reserved2, RESERVED2 )
    };

    //
    // End .h

    // Begin .cpp
    //
    
    DEFINE_SINGLETON_RESOURCE( TestResource )

    //
    // End .cpp

    BOOST_FIXTURE_TEST_SUITE(StringResourceTestSuite,StringResourceTest)

    BOOST_AUTO_TEST_CASE(BasicGet)
    {
        auto id1 = IDS_COMMON_RESERVED1;
        auto id2 = IDS_COMMON_RESERVED2;

        Trace.WriteWarning(TraceComponent, "Value 1: '{0}'", StringResource::Get(id1));
        Trace.WriteWarning(TraceComponent, "Value 2: '{0}'", StringResource::Get(id2));

        VERIFY_IS_TRUE(!StringResource::Get(id1).empty());
        VERIFY_IS_TRUE(!StringResource::Get(id2).empty());

        VERIFY_IS_TRUE(StringResource::Get(id1) != StringResource::Get(id2));
    }

    BOOST_AUTO_TEST_CASE(BasicGetBufferIncrease)
    {
        CommonConfig::GetConfig().MinResourceStringBufferSizeInWChars = 2;
        CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars = 128;

        auto id1 = IDS_COMMON_RESERVED1;
        auto id2 = IDS_COMMON_RESERVED2;

        Trace.WriteWarning(TraceComponent, "Value 1: '{0}'", StringResource::Get(id1));
        Trace.WriteWarning(TraceComponent, "Value 2: '{0}'", StringResource::Get(id2));

        VERIFY_IS_TRUE(!StringResource::Get(id1).empty());
        VERIFY_IS_TRUE(!StringResource::Get(id2).empty());

        VERIFY_IS_TRUE(StringResource::Get(id1).size() > static_cast<size_t>(CommonConfig::GetConfig().MinResourceStringBufferSizeInWChars));
        VERIFY_IS_TRUE(StringResource::Get(id2).size() > static_cast<size_t>(CommonConfig::GetConfig().MinResourceStringBufferSizeInWChars));

        VERIFY_IS_TRUE(StringResource::Get(id1).size() < static_cast<size_t>(CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars));
        VERIFY_IS_TRUE(StringResource::Get(id2).size() < static_cast<size_t>(CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars));

        VERIFY_IS_TRUE(StringResource::Get(id1) != StringResource::Get(id2));
    }

    BOOST_AUTO_TEST_CASE(BasicGetTruncation)
    {
        CommonConfig::GetConfig().MinResourceStringBufferSizeInWChars = 2;
        CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars = 16;

        auto id1 = IDS_COMMON_RESERVED1;
        auto id2 = IDS_COMMON_RESERVED2;

        Trace.WriteWarning(TraceComponent, "Value 1: '{0}'", StringResource::Get(id1));
        Trace.WriteWarning(TraceComponent, "Value 2: '{0}'", StringResource::Get(id2));

        VERIFY_IS_TRUE(StringResource::Get(id1).size() == static_cast<size_t>(CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars - 1));
        VERIFY_IS_TRUE(StringResource::Get(id2).size() == static_cast<size_t>(CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars - 1));

        VERIFY_IS_TRUE(StringResource::Get(id1) != StringResource::Get(id2));
    }

    BOOST_AUTO_TEST_CASE(BasicGetInvalid)
    {
        auto id1 = 0;

        Trace.WriteWarning(TraceComponent, "Value 1: '{0}'", StringResource::Get(id1));

        VERIFY_IS_TRUE(StringResource::Get(id1).empty());
    }

    BOOST_AUTO_TEST_CASE(ResourceComponent)
    {
        Trace.WriteWarning(TraceComponent, "Value 1: '{0}'", TestResource::GetResources().Reserved1);
        Trace.WriteWarning(TraceComponent, "Value 2: '{0}'", TestResource::GetResources().Reserved2);

        VERIFY_IS_TRUE(!TestResource::GetResources().Reserved1.empty());
        VERIFY_IS_TRUE(!TestResource::GetResources().Reserved2.empty());

        VERIFY_IS_TRUE(TestResource::GetResources().Reserved1 != TestResource::GetResources().Reserved2);
    }

    BOOST_AUTO_TEST_CASE(ResourceComponentBufferIncrease)
    {
        CommonConfig::GetConfig().MinResourceStringBufferSizeInWChars = 2;
        CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars = 128;

        Trace.WriteWarning(TraceComponent, "Value 1: '{0}'", TestResource::GetResources().Reserved1);
        Trace.WriteWarning(TraceComponent, "Value 2: '{0}'", TestResource::GetResources().Reserved2);

        VERIFY_IS_TRUE(!TestResource::GetResources().Reserved1.empty());
        VERIFY_IS_TRUE(!TestResource::GetResources().Reserved2.empty());

        VERIFY_IS_TRUE(TestResource::GetResources().Reserved1.size() > static_cast<size_t>(CommonConfig::GetConfig().MinResourceStringBufferSizeInWChars));
        VERIFY_IS_TRUE(TestResource::GetResources().Reserved2.size() > static_cast<size_t>(CommonConfig::GetConfig().MinResourceStringBufferSizeInWChars));

        VERIFY_IS_TRUE(TestResource::GetResources().Reserved1.size() < static_cast<size_t>(CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars));
        VERIFY_IS_TRUE(TestResource::GetResources().Reserved2.size() < static_cast<size_t>(CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars));

        VERIFY_IS_TRUE(TestResource::GetResources().Reserved1 != TestResource::GetResources().Reserved2);
    }

    BOOST_AUTO_TEST_CASE(ResourceComponentTruncation)
    {
        CommonConfig::GetConfig().MinResourceStringBufferSizeInWChars = 2;
        CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars = 16;

        Trace.WriteWarning(TraceComponent, "Value 1: '{0}'", TestResource::GetResources().Reserved1);
        Trace.WriteWarning(TraceComponent, "Value 2: '{0}'", TestResource::GetResources().Reserved2);

        VERIFY_IS_TRUE(TestResource::GetResources().Reserved1.size() == static_cast<size_t>(CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars - 1));
        VERIFY_IS_TRUE(TestResource::GetResources().Reserved2.size() == static_cast<size_t>(CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars - 1));

        VERIFY_IS_TRUE(TestResource::GetResources().Reserved1 != TestResource::GetResources().Reserved2);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool StringResourceTest::TestReset()
    {
        CommonConfig::Test_Reset();
        TestResource::Test_Reset();
        return true;
    }
}
