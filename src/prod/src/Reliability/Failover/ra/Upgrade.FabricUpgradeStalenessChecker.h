// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            class FabricUpgradeStalenessChecker
            {
            public:
                FabricUpgradeStalenessCheckResult::Enum CheckFabricUpgradeAtNodeUp(
                    Common::FabricVersionInstance const & nodeVersion,
                    Common::FabricVersionInstance const & incomingVersion);

                FabricUpgradeStalenessCheckResult::Enum CheckFabricUpgradeAtUpgradeMessage(
                    Common::FabricVersionInstance const & nodeVersion,
                    Common::FabricVersionInstance const & incomingVersion);
            };
        }
    }
}
