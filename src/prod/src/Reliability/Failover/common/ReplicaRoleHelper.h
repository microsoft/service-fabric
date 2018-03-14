// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicaRole
    {
        Reliability::LoadBalancingComponent::ReplicaRole::Enum ConvertToPLBReplicaRole(bool isStateful, ReplicaRole::Enum toConvert);
    }
}


