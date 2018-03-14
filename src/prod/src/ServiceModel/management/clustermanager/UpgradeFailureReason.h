// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        namespace UpgradeFailureReason
        {
            enum Enum
            {
                None = 0,
                Interrupted = 1,
                HealthCheck = 2,
                UpgradeDomainTimeout = 3,
                OverallUpgradeTimeout = 4,
                ProcessingFailure = 5,
                LastValue = ProcessingFailure,
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE( UpgradeFailureReason )

            Enum FromPublicApi(FABRIC_UPGRADE_FAILURE_REASON);
            FABRIC_UPGRADE_FAILURE_REASON ToPublicApi(Enum const);
        };
    }
}
