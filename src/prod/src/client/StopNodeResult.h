// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class StopNodeResult
        : public Api::IStopNodeResult
        , public Common::ComponentRoot
    {
        DENY_COPY(StopNodeResult);

    public:
        StopNodeResult(
            __in Management::FaultAnalysisService::StopNodeStatus && nodeStatus);

        std::shared_ptr<Management::FaultAnalysisService::NodeResult> const & GetNodeResult() override;

    private:
        Management::FaultAnalysisService::StopNodeStatus nodeStatus_;
    };
}
