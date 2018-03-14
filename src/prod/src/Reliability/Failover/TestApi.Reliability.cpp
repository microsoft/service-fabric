// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Failover.stdafx.h"
#include "TestApi.Failover.Internal.h"
#include "Reliability/Failover/fm/TestApi.FM.h"
#include "Reliability/Failover/ra/TestApi.RA.h"
#include "TestApi.Reliability.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReliabilityTestApi;

ReliabilityTestHelper::ReliabilityTestHelper(IReliabilitySubsystem & reliability)
: reliability_(dynamic_cast<ReliabilitySubsystem&>(reliability))
{
}

IReliabilitySubsystem & ReliabilityTestHelper::get_Reliability() const 
{ 
    return reliability_; 
}

FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr ReliabilityTestHelper::GetFailoverManager()
{
    auto fm = reliability_.Test_GetFailoverManager();

    if (!fm)
    {
        return FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr();
    }

    return make_unique<FailoverManagerComponentTestApi::FailoverManagerTestHelper>(fm);
}

FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr ReliabilityTestHelper::GetFailoverManagerMaster()
{
    auto fmm = reliability_.Test_GetFailoverManagerMaster();

    if (!fmm)
    {
        return FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr();
    }

    return make_unique<FailoverManagerComponentTestApi::FailoverManagerTestHelper>(fmm);
}

ReconfigurationAgentComponentTestApi::ReconfigurationAgentTestHelperUPtr ReliabilityTestHelper::GetRA()
{
    return make_unique<ReconfigurationAgentComponentTestApi::ReconfigurationAgentTestHelper>(reliability_.ReconfigurationAgent);
}

ErrorCode ReliabilityTestHelper::DeleteAllFMStoreData()
{
    return FailoverManagerComponentTestApi::FailoverManagerTestHelper::DeleteAllStoreData();
}
