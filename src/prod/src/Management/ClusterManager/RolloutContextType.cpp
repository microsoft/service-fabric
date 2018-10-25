// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace ClusterManager
    {
        namespace RolloutContextType
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                    case Invalid: w << "Invalid"; return;
                    case ApplicationType: w << "ApplicationTypeContext"; return;
                    case Application: w << "ApplicationContext"; return;
                    case Service: w << "ServiceContext"; return;
                    case ApplicationUpgrade: w << "ApplicationUpgradeContext"; return;
                    case FabricProvision: w << "FabricProvisionContext"; return;
                    case FabricUpgrade: w << "FabricUpgradeContext"; return;
                    case InfrastructureTask: w << "InfrastructureTask"; return;
                    case ApplicationUpdate: w << "ApplicationUpdate"; return;
                    case ComposeDeployment: w << "ComposeDeployment"; return;
                    case ComposeDeploymentUpgrade: w << "ComposeDeploymentUpgrade"; return;
                    case SingleInstanceDeployment: w << "SingleInstanceDeployment"; return;
                    case SingleInstanceDeploymentUpgrade: w << "SingleInstanceDeploymentUpgrade"; return;
                };

                w << "RolloutContextType(" << static_cast<int>(e) << ')';
            }
        }
    }
}
