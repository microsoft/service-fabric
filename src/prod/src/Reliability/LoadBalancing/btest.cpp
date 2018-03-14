// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PLBConfig.h"
#if defined(PLATFORM_UNIX)
#define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include "PlacementAndLoadBalancing.h"
bool initBoost()
{
    return true;
}

int BOOST_TEST_CALL_DECL
main(int argc, char* argv[])
{
    vector<char*> filteredArgv;

    bool useAppGroupsInBoost = false;
    bool testMode = false;
    bool useRGInBoost = false;
    wstring traceFileExtension = L"";

    for (int i = 0; i < argc; ++i)
    {
        if (Common::StringUtility::CompareCaseInsensitive(argv[i], "-UseAppGroupsInBoost") == 0)
        {
            useAppGroupsInBoost = true;
            traceFileExtension += L".AppGroups";
        }
        else if (Common::StringUtility::CompareCaseInsensitive(argv[i], "-UseTestMode") == 0)
        {
            testMode = true;
            traceFileExtension += L".TestMode";
        }
        else if (Common::StringUtility::CompareCaseInsensitive(argv[i], "-UseRGInBoost") == 0)
        {
            useRGInBoost = true;
            traceFileExtension += L".RG";
        }
        else
        {
            filteredArgv.push_back(argv[i]);
        }
    }


    if (traceFileExtension.size() > 0)
    {
        // In case we are running multiple test flavors in parallel
        Common::TraceTextFileSink::SetPath(L"LoadBalancing.Test" + traceFileExtension + L".trace");
    }

    Reliability::LoadBalancingComponent::PLBConfig::GetConfig().UseAppGroupsInBoost = useAppGroupsInBoost;
    Reliability::LoadBalancingComponent::PLBConfig::GetConfig().UseRGInBoost = useRGInBoost;
    Reliability::LoadBalancingComponent::PLBConfig::GetConfig().IsTestMode = testMode;

    Common::TraceConsoleSink::Write(Common::LogLevel::Info,
        Common::wformatString(L"Starting tests with IsTestMode={0} UseAppGroupInBoost={1} UseRGInBoost={2}",
            testMode,
            useAppGroupsInBoost,
            useRGInBoost));

    // argv[0] is always the program name, so filteredArgv will always have at least one element.
    auto error = ::boost::unit_test::unit_test_main(&initBoost, static_cast<int>(filteredArgv.size()), &(filteredArgv[0]));

    return error;
}
