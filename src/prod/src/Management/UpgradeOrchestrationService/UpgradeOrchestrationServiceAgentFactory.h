// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace UpgradeOrchestrationService
    {
        class UpgradeOrchestrationServiceAgentFactory
            : public Api::IUpgradeOrchestrationServiceAgentFactory
            , public Common::ComponentRoot
        {
        public:
            virtual ~UpgradeOrchestrationServiceAgentFactory();

            static Api::IUpgradeOrchestrationServiceAgentFactoryPtr Create();

            virtual Common::ErrorCode CreateUpgradeOrchestrationServiceAgent(__out Api::IUpgradeOrchestrationServiceAgentPtr &);

        private:
            UpgradeOrchestrationServiceAgentFactory();
        };
    }
}

