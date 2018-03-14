// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IUpgradeOrchestrationServiceAgentFactory
    {
    public:
        virtual ~IUpgradeOrchestrationServiceAgentFactory() {};

        virtual Common::ErrorCode CreateUpgradeOrchestrationServiceAgent(
            __out IUpgradeOrchestrationServiceAgentPtr &) = 0;
    };
}
