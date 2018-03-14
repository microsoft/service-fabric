// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        namespace InfrastructureTaskState
        {
            enum Enum : int
            {
                Invalid = 0,

                PreProcessing = 1,
                PreAckPending = 2,
                PreAcked = 3,

                PostProcessing = 4,
                PostAckPending = 5,
                PostAcked = 6,
                LAST_ENUM_PLUS_ONE = 7,
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE( InfrastructureTaskState )

            Enum FromPublicApi(FABRIC_INFRASTRUCTURE_TASK_STATE);
            FABRIC_INFRASTRUCTURE_TASK_STATE ToPublicApi(Enum const);
        };
    }
}

