// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReplicaUp
        {
            // Defines whether the replica is to be uploaded immediately
            // or deferred i.e. wait for the reopen to complete before upload
            namespace ReplicaUploadType
            {
                enum Enum
                {
                    Immediate = 0,
                    Deferred = 1,
                };
            };

            typedef std::map<FailoverUnitId, ReplicaUploadType::Enum> ToBeUploadedReplicaSet;
        }
    }
}

