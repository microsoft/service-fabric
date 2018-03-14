// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class RestartNodeResult
        : public Api::IRestartNodeResult
        , public Common::ComponentRoot
    {
        DENY_COPY(RestartNodeResult);

    public:
        RestartNodeResult(
            __in Management::FaultAnalysisService::RestartNodeStatus && nodeStatus);

        std::shared_ptr<Management::FaultAnalysisService::NodeResult> const & GetNodeResult();

    private:
        Management::FaultAnalysisService::RestartNodeStatus nodeStatus_;
    };
}
