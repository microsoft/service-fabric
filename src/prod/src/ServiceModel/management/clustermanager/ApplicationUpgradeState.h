// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        namespace ApplicationUpgradeState
        {
            enum Enum
            {
                Invalid = 0,
                CompletedRollforward = 1,
                CompletedRollback = 2,
                RollingForward = 3,
                RollingBack = 4,

                // Indicates that rollback (specifically due to upgrade interruption) has completed.
                // Allows special handling for the scenario: 
                //
                // V1->V2, interrupt V1
                // 
                // The CM can accept the interrupting V1 request with success and put the upgrade into
                // a CompletedRollforward state rather than leaving it in a CompletedRollback state.
                // This is unnecessary with an explicit Rollback() API.
                //
                Interrupted = 5,

                Failed = 6,

                FirstValidEnum = Invalid,
                LastValidEnum = Failed
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE( ApplicationUpgradeState)
        };
    }
}

