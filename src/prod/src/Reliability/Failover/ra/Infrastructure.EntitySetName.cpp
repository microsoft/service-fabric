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

void EntitySetName::WriteToTextWriter(TextWriter & w, EntitySetName::Enum e)
{
    switch (e)
    {
    case EntitySetName::Invalid:
        w << "Invalid";
        break;

    case EntitySetName::ReconfigurationMessageRetry:
        w << "ReconfigurationMessageRetry";
        return;

    case EntitySetName::StateCleanup:
        w << "StateCleanup";
        return; 

    case EntitySetName::ReplicaDown:
        w << "ReplicaDown";
        return;

    case EntitySetName::ReplicaCloseMessageRetry:
        w << "ReplicaCloseMessageRetry";
        return;

    case EntitySetName::ReplicaUpMessageRetry:
        w << "ReplicaUpMessageRetry";
        return;

    case EntitySetName::ReplicaOpenMessageRetry:
        w << "ReplicaOpenMessageRetry";
        return;

    case EntitySetName::UpdateServiceDescriptionMessageRetry:
        w << "UpdateServiceDescriptionMessageRetry";
        return;

    case EntitySetName::FailoverManagerMessageRetry:
        w << "FailoverManagerMessageRetry";
        return;

    case EntitySetName::ReplicaUploadPending:
        w << "ReplicaUploadPending";
        break;

    case EntitySetName::Test:
        w << "Test";
        break;

    default:
        Assert::CodingError("unknown set name {0}", static_cast<int>(e));
    };
}
