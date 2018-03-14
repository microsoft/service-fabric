// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IOrchestrationUpgradeStatusResult
    {
    public:
        virtual ~IOrchestrationUpgradeStatusResult() {};

        virtual std::shared_ptr<Management::UpgradeOrchestrationService::OrchestrationUpgradeProgress> const & GetProgress() = 0;
    };
}
