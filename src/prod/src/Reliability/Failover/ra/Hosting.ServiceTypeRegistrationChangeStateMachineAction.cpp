// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ReconfigurationAgentComponent::Infrastructure;
using namespace Hosting;
using namespace Hosting2;

ServiceTypeRegistrationChangeStateMachineAction::ServiceTypeRegistrationChangeStateMachineAction(
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    bool isAdd) :
    registration_(registration),
    isAdd_(isAdd)
{
}

void ServiceTypeRegistrationChangeStateMachineAction::OnPerformAction(
    std::wstring const& activityId, 
    EntityEntryBaseSPtr const & entity,
    ReconfigurationAgent& reconfigurationAgent)
{
    UNREFERENCED_PARAMETER(entity);

    auto & hosting = reconfigurationAgent.HostingAdapterObj;
    if (isAdd_)
    {
        hosting.OnServiceTypeRegistrationAdded(activityId, registration_);
    }
    else
    {
        hosting.OnServiceTypeRegistrationRemoved(activityId, registration_);
    }
}

void ServiceTypeRegistrationChangeStateMachineAction::OnCancelAction(
    ReconfigurationAgent& )
{

}
