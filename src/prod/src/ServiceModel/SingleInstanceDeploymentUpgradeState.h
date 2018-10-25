//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace SingleInstanceDeploymentUpgradeState
    {
        enum class Enum
        {
            Invalid = 0,
            ProvisioningTarget = 1,
            RollingForward = 2,
            UnprovisioningCurrent = 3,
            CompletedRollforward = 4,
            RollingBack = 5,
            UnprovisioningTarget = 6,
            CompletedRollback = 7,
            Failed = 8,

            FirstValidEnum = Invalid,
            LastValidEnum = Failed
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}
