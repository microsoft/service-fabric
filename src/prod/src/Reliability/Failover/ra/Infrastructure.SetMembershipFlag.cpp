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
using namespace Infrastructure;

SetMembershipFlag::SetMembershipFlag(EntitySetIdentifier const & id)
: id_(id),
  isSet_(false)
{
}

void SetMembershipFlag::SetValue(bool value, StateMachineActionQueue & queue)
{
    if (isSet_ && value)
    {
        return;
    }

    if (!isSet_ && !value)
    {
        return;
    }

    isSet_ = value;
    queue.Enqueue(make_unique<EntitySet::ChangeSetMembershipAction>(id_, value));
}       
