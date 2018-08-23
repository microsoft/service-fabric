// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace UpgradeOrchestrationService
    {
        namespace OrchestrationUpgradeState
        {
            enum Enum
            {
                Invalid = 0,
                RollingBackInProgress = 1,
                RollingBackCompleted = 2,
                RollingForwardPending = 3,
                RollingForwardInProgress = 4,
                RollingForwardCompleted = 5,
                Failed = 6,
                RollingBackPending = 7,
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
            Common::ErrorCode FromPublicApi(FABRIC_UPGRADE_STATE const & state, __out Enum & progress);
            FABRIC_UPGRADE_STATE ToPublicApi(Enum const & state);
        };
    }
}
