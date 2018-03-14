// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class StartNodeResult
        : public Api::IStartNodeResult
        , public Common::ComponentRoot
    {
        DENY_COPY(StartNodeResult);

    public:
        StartNodeResult(
            __in Management::FaultAnalysisService::StartNodeStatus && nodeStatus);

        std::shared_ptr<Management::FaultAnalysisService::NodeResult> const & GetNodeResult() override;

    private:
        Management::FaultAnalysisService::StartNodeStatus nodeStatus_;
    };
}
