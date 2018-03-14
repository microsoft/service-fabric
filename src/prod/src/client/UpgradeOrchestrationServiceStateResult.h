// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class UpgradeOrchestrationServiceStateResult
        : public Api::IUpgradeOrchestrationServiceStateResult
        , public Common::ComponentRoot
    {
        DENY_COPY(UpgradeOrchestrationServiceStateResult);

    public:
		UpgradeOrchestrationServiceStateResult(
            __in std::shared_ptr<Management::UpgradeOrchestrationService::UpgradeOrchestrationServiceState> &&state);

        std::shared_ptr<Management::UpgradeOrchestrationService::UpgradeOrchestrationServiceState> const & GetState();

    private:
        std::shared_ptr<Management::UpgradeOrchestrationService::UpgradeOrchestrationServiceState> state_;
    };
}
