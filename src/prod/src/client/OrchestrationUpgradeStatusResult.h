// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class OrchestrationUpgradeStatusResult
        : public Api::IOrchestrationUpgradeStatusResult
        , public Common::ComponentRoot
    {
        DENY_COPY(OrchestrationUpgradeStatusResult);

    public:
        OrchestrationUpgradeStatusResult(
            __in std::shared_ptr<Management::UpgradeOrchestrationService::OrchestrationUpgradeProgress> &&progress);

        std::shared_ptr<Management::UpgradeOrchestrationService::OrchestrationUpgradeProgress> const & GetProgress();

    private:
        std::shared_ptr<Management::UpgradeOrchestrationService::OrchestrationUpgradeProgress> progress_;
    };
}
