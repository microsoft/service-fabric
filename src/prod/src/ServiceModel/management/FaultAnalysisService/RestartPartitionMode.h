// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace FaultAnalysisService
    {
        namespace RestartPartitionMode
        {
            enum Enum
            {
                Invalid = 0,
                AllReplicasOrInstances = 1,
                OnlyActiveReplicas = 2,
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            Common::ErrorCode FromPublicApi(FABRIC_RESTART_PARTITION_MODE const & publicMode, __out Enum & mode);
            FABRIC_RESTART_PARTITION_MODE ToPublicApi(Enum const & mode);
        };
    }
}
