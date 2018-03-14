// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;

StateMachineAction::StateMachineAction()
{
}

void StateMachineAction::PerformAction(
    wstring const & activityId, 
    EntityEntryBaseSPtr const & entity, 
    ReconfigurationAgent & reconfigurationAgent)
{
    OnPerformAction(activityId, entity, reconfigurationAgent);
}

void StateMachineAction::CancelAction(ReconfigurationAgent & ra)
{
    OnCancelAction(ra);
}
