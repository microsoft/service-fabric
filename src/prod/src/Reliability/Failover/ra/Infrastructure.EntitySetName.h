// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            namespace EntitySetName
            {
                enum Enum
                {
                    Invalid,
                    ReconfigurationMessageRetry,
                    StateCleanup,
                    ReplicaDown,
                    ReplicaCloseMessageRetry,
                    ReplicaUpMessageRetry,
                    ReplicaOpenMessageRetry,
                    UpdateServiceDescriptionMessageRetry,
                    FailoverManagerMessageRetry,
                    ReplicaUploadPending,
                    Test,
                };

                void WriteToTextWriter(Common::TextWriter & w, Enum e);
            }
        }
    }
}
