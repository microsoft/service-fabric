// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        namespace RolloutContextType
        {
            enum Enum
            {
                Invalid = 0,
                ApplicationType = 1,
                Application = 2,
                Service = 3,
                ApplicationUpgrade = 4,
                FabricProvision = 5,
                FabricUpgrade = 6,
                InfrastructureTask = 7,
                ApplicationUpdate = 8,
                ComposeDeployment = 9,
                ComposeDeploymentUpgrade = 10 // TODO rename these to instance based deployment
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
        };
    }
}
