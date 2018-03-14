// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ReconfigurationAgentComponent::Communication;

RequestFMMessageRetryAction::RequestFMMessageRetryAction(Reliability::FailoverManagerId const & target) :
target_(target)
{
}

void RequestFMMessageRetryAction::OnPerformAction(
    std::wstring const & activityId,
    Infrastructure::EntityEntryBaseSPtr const & entity,
    ReconfigurationAgent & ra) 
{
    UNREFERENCED_PARAMETER(entity);
    ra.GetFMMessageRetryComponent(target_).Request(activityId);
}

void RequestFMMessageRetryAction::OnCancelAction(
        ReconfigurationAgent & ra) 
{
    UNREFERENCED_PARAMETER(ra);
}
