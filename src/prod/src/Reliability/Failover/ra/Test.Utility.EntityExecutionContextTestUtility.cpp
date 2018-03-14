// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Infrastructure;

EntityExecutionContextTestUtility::EntityExecutionContextTestUtility(ScenarioTest & test) :
    test_(test)
{
}

CommitTypeDescriptionUtility EntityExecutionContextTestUtility::ExecuteEntityProcessor(
    std::wstring const & ftShortName, 
    EntityProcessor const & processor)
{
    auto entry = test_.GetLFUMEntry(ftShortName);

    Infrastructure::StateMachineActionQueue queue;

    CommitTypeDescriptionUtility rv;

    {
        auto lock = entry->Test_CreateLock();
        Infrastructure::EntityExecutionContext context = Infrastructure::EntityExecutionContext::Create(
            test_.RA, 
            queue, 
            lock.UpdateContextObj,
            lock.Current);

        processor(context);


        if (lock.IsUpdating)
        {
            rv = CommitTypeDescriptionUtility(lock.CreateCommitDescription().Type);
        }
    }

    queue.ExecuteAllActions(DefaultActivityId, entry, test_.RA);

    return rv;
}
