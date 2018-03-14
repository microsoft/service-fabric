// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Hosting;

TerminateHostStateMachineAction::TerminateHostStateMachineAction(
    TerminateServiceHostReason::Enum reason,
    Hosting2::ServiceTypeRegistrationSPtr const & registration) :
    reason_(reason),
    registration_(registration)
{
    ASSERT_IF(registration == nullptr, "Registration can't ben null");
}

void TerminateHostStateMachineAction::OnPerformAction(
    std::wstring const & activityId, 
    Infrastructure::EntityEntryBaseSPtr const &, 
    ReconfigurationAgent & ra)
{
    ra.HostingAdapterObj.TerminateServiceHost(activityId, reason_, registration_);
}

void TerminateHostStateMachineAction::OnCancelAction(ReconfigurationAgent &)
{
}
