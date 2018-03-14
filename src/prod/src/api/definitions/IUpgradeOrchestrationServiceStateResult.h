// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IUpgradeOrchestrationServiceStateResult
    {
    public:
        virtual ~IUpgradeOrchestrationServiceStateResult() {};

        virtual std::shared_ptr<Management::UpgradeOrchestrationService::UpgradeOrchestrationServiceState> const & GetState() = 0;
    };
}
