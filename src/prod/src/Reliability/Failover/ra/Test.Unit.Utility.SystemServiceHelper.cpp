// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ReliabilityUnitTest;
using namespace ServiceModel;
using namespace Utility2;

namespace
{
    ServiceDescription GetFMServiceDescription()
    {
        return StateManagement::Default::GetInstance().FM_STContext.SD;
    }

    ServiceDescription GetAdHocServiceDescription()
    {
        return StateManagement::Default::GetInstance().ASP1_STContext.SD;
    }

    ServiceDescription GetUserServiceDescription()
    {
        return StateManagement::Default::GetInstance().SP2_SP2_STContext.SD;
    }

    ServiceDescription GetRMServiceDescription()
    {
        return StateManagement::Default::GetInstance().RM_STContext.SD;
    }

    FailoverUnitDescription GetFMFTDescription()
    {        
        return StateManagement::Default::GetInstance().FmFTContext.GetFailoverUnitDescription();
    }

    FailoverUnitDescription GetRMFTDescription()
    {
        return StateManagement::Default::GetInstance().RM_FTContext.GetFailoverUnitDescription();
    }

    FailoverUnitDescription GetOtherFTDescription()
    {
        return FailoverUnitDescription(
            FailoverUnitId(),
            ConsistencyUnitDescription(ConsistencyUnitId()));
    }
}

class TestSystemServiceHelper
{
protected:
    void TestHelper(
        FailoverUnitDescription const & ftDesc,
        ServiceDescription const & serviceDesc,
        SystemServiceHelper::HostType::Enum expectedHostType,
        SystemServiceHelper::ServiceType::Enum expectedServiceType)
    {
        VERIFY_ARE_EQUAL(expectedHostType, Utility2::SystemServiceHelper::GetHostType(ftDesc, serviceDesc.ApplicationName));
        VERIFY_ARE_EQUAL(expectedServiceType, Utility2::SystemServiceHelper::GetServiceType(serviceDesc.Name));
    }
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestSystemServiceHelperSuite,TestSystemServiceHelper)

BOOST_AUTO_TEST_CASE(FMService)
{
    TestHelper(
        GetFMFTDescription(),
        GetFMServiceDescription(),
        SystemServiceHelper::HostType::Fabric,
        SystemServiceHelper::ServiceType::System);
}

BOOST_AUTO_TEST_CASE(AdHocService)
{
    TestHelper(
        GetOtherFTDescription(),
        GetAdHocServiceDescription(),
        SystemServiceHelper::HostType::AdHoc,
        SystemServiceHelper::ServiceType::User);
}

BOOST_AUTO_TEST_CASE(RMService)
{
    TestHelper(
        GetRMFTDescription(),
        GetRMServiceDescription(),
        SystemServiceHelper::HostType::Managed,
        SystemServiceHelper::ServiceType::System);
}

BOOST_AUTO_TEST_CASE(UserService)
{
    TestHelper(
        GetOtherFTDescription(),
        GetUserServiceDescription(),
        SystemServiceHelper::HostType::Managed,
        SystemServiceHelper::ServiceType::User);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
