// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ServiceModelTests
{
    using namespace std;
    using namespace Common;
    using namespace ServiceModel;

    StringLiteral const TestSource("ApplicationIdentifierTest");

    class ApplicationIdentifierTest
    {
    protected:
        void VerifyToString(ApplicationIdentifier const & appId, wstring const & expectedToString);
        void VerifyConversion(ApplicationIdentifier const & appId);
    };

    BOOST_FIXTURE_TEST_SUITE(ApplicationIdentifierTestSuite,ApplicationIdentifierTest)

    BOOST_AUTO_TEST_CASE(ToStringTest)
    {
        VerifyToString(ApplicationIdentifier(), L"");
        VerifyToString(ApplicationIdentifier(L"CalculatorApp", 0), L"CalculatorApp_App0");
        VerifyToString(ApplicationIdentifier(L"MyAppType", 23), L"MyAppType_App23");
        VerifyToString(ApplicationIdentifier(L"My_AppType", 23), L"My_AppType_App23");
        VerifyToString(ApplicationIdentifier(L"My_AppType_App23", 23), L"My_AppType_App23_App23");
    }


    BOOST_AUTO_TEST_CASE(ConversionTest)
    {
        VerifyConversion(ApplicationIdentifier());
        VerifyConversion(ApplicationIdentifier(L"CalculatorApp", 0));
        VerifyConversion(ApplicationIdentifier(L"MyAppType", 23));
        VerifyConversion(ApplicationIdentifier(L"My_AppType", 23));
        VerifyConversion(ApplicationIdentifier(L"My_AppType_App23", 23));

        ApplicationIdentifier convertedAppId;
        auto error = ApplicationIdentifier::FromString(L"SomeThing", convertedAppId);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"ApplicationIdentifier::FromString(SomeThing)");

        error = ApplicationIdentifier::FromString(L"SomeThing_App", convertedAppId);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"ApplicationIdentifier::FromString(SomeThing_App)");

        error = ApplicationIdentifier::FromString(L"SomeThing_AppX", convertedAppId);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"ApplicationIdentifier::FromString(SomeThing_AppX)");

        error = ApplicationIdentifier::FromString(L"SomeThing_AppX_App345", convertedAppId);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ApplicationIdentifier::FromString(SomeThing_AppX_App345)");

        error = ApplicationIdentifier::FromString(L"SomeThing_App0_App1", convertedAppId);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ApplicationIdentifier::FromString(SomeThing_App0_App1)");
        VERIFY_IS_TRUE(convertedAppId.ApplicationNumber == 1, L"convertedAppId.ApplicationNumber == 1");

        error = ApplicationIdentifier::FromString(L"SomeThing_App0_AppX", convertedAppId);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"ApplicationIdentifier::FromString(SomeThing_App0_AppX)");
    }



    BOOST_AUTO_TEST_CASE(ComparisionTest)
    {
        VERIFY_IS_TRUE(ApplicationIdentifier() < ApplicationIdentifier(L"MyAppType",0), L"ApplicationIdentifier() < ApplicationIdentifier(L\"MyAppType\",0)");
        VERIFY_IS_FALSE(ApplicationIdentifier(L"MyAppType",0) < ApplicationIdentifier(L"MyAppType",0), L"ApplicationIdentifier(L\"MyAppType\",0) < ApplicationIdentifier(L\"MyAppType\",0)");
        VERIFY_IS_TRUE(ApplicationIdentifier(L"MyAppType",0) < ApplicationIdentifier(L"MyAppType",3), L"ApplicationIdentifier(L\"MyAppType\",0) < ApplicationIdentifier(L\"MyAppType\",3)");
        VERIFY_IS_FALSE(ApplicationIdentifier(L"ZyAppType",3) < ApplicationIdentifier(L"MyAppType",3), L"ApplicationIdentifier(L\"MyAppType\",5) < ApplicationIdentifier(L\"MyAppType\",3)");
    }

    BOOST_AUTO_TEST_SUITE_END()

    void ApplicationIdentifierTest::VerifyToString(ApplicationIdentifier const & appId, wstring const & expectedToString)
    {
        VERIFY_ARE_EQUAL(appId.ToString(), expectedToString);
    }

    void ApplicationIdentifierTest::VerifyConversion(ApplicationIdentifier const & originalAppId)
    {
        wstring originalAppIdString = originalAppId.ToString();

        ApplicationIdentifier convertedAppId;
        auto error = ApplicationIdentifier::FromString(originalAppIdString, convertedAppId);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ApplicationIdentifier::FromString");
        wstring convertedAppIdString = convertedAppId.ToString();

        Trace.WriteNoise(TestSource, "OriginalAppIdString={0}, ConvertedAppIdString={1}", error);

        VERIFY_IS_TRUE(originalAppId == convertedAppId, L"OriginalAppId == ConvertedAppId");
        VERIFY_IS_TRUE(convertedAppId == originalAppId, L"ConvertedAppId == OriginalAppId");
        VERIFY_IS_TRUE(originalAppIdString == convertedAppIdString, L"OriginalAppIdString == ConvertedAppIdString");
    }

}
