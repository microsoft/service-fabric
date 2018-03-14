// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;

void ReplicaProxy::WriteTo(TextWriter& writer, FormatOptions const &) const
{
    writer.Write("{0}:{1} {2} {3} {4}",
        failoverUnitProxyState_,
        replicatorState_,
        static_cast<bool>(buildIdleOperation_.IsRunning) ? L"1" : L"0",
        stateUpdateTime_,
        replicaDescription_);
}

void ReplicaProxy::WriteToEtw(uint16 contextSequenceId) const
{
    RAPEventSource::Events->ReplicaProxy(
        contextSequenceId,
        failoverUnitProxyState_,
        replicatorState_,
        static_cast<bool>(buildIdleOperation_.IsRunning),
        stateUpdateTime_,
        replicaDescription_);        
}
