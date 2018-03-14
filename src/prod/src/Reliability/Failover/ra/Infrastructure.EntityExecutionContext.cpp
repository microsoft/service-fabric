// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

EntityExecutionContext EntityExecutionContext::Create(
    ReconfigurationAgent & ra, 
    StateMachineActionQueue & queue, 
    UpdateContext & updateContext,
    IEntityStateBase const * state)
{
    return EntityExecutionContext(
        ra.Config,
        ra.Clock,
        ra.HostingAdapterObj,
        queue,
        ra.NodeInstance,
        updateContext,
        state);
}
