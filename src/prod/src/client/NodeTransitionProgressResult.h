// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class NodeTransitionProgressResult
        : public Api::INodeTransitionProgressResult
        , public Common::ComponentRoot
    {
        DENY_COPY(NodeTransitionProgressResult);

    public:
        NodeTransitionProgressResult(
            __in std::shared_ptr<Management::FaultAnalysisService::NodeTransitionProgress> &&progress);

        std::shared_ptr<Management::FaultAnalysisService::NodeTransitionProgress> const & GetProgress();

    private:
        std::shared_ptr<Management::FaultAnalysisService::NodeTransitionProgress> progress_;
    };
}
