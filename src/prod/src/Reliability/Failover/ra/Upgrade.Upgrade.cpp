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
using namespace ReconfigurationAgentComponent::Upgrade;
using namespace Infrastructure;

namespace
{
    StringLiteral const Started("Started");
    StringLiteral const Cancelled("Cancelled");
    StringLiteral const Completed("Completed");

    void TraceAsyncOperation(
        ReconfigurationAgent& ra, 
        wstring const & activityId, 
        StringLiteral const & tag,
        UpgradeStateName::Enum state, 
        AsyncOperation & op, 
        Common::ErrorCode const & error)
    {
        RAEventSource::Events->UpgradeAsyncOperationStateChange(
            ra.NodeInstanceIdStr,
            activityId,
            tag,
            state,
            error,
            reinterpret_cast<uintptr_t>(&op));
    }
}

void IUpgrade::TraceAsyncOperationCancel(UpgradeStateName::Enum state, AsyncOperation & op)
{
    TraceAsyncOperation(ra_, GetActivityId(), Cancelled, state, op, ErrorCodeValue::Success);
}

void IUpgrade::TraceAsyncOperationStart(UpgradeStateName::Enum state, AsyncOperation & op)
{
    TraceAsyncOperation(ra_, GetActivityId(), Started, state, op, ErrorCodeValue::Success);
}

void IUpgrade::TraceAsyncOperationEnd(UpgradeStateName::Enum state, AsyncOperation & op, Common::ErrorCode const & error)
{
    TraceAsyncOperation(ra_, GetActivityId(), Completed, state, op, error);
}

void IUpgrade::TraceLifeCycle(Common::StringLiteral const & tag)
{
    RAEventSource::Events->UpgradeLifeCycle(ra_.NodeInstanceIdStr, GetActivityId(), tag, Common::TraceCorrelatedEvent<IUpgrade>(*this));
}
