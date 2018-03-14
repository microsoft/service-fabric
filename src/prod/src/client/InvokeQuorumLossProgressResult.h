// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class InvokeQuorumLossProgressResult
        : public Api::IInvokeQuorumLossProgressResult
        , public Common::ComponentRoot
    {
        DENY_COPY(InvokeQuorumLossProgressResult);

    public:
        InvokeQuorumLossProgressResult(
            __in std::shared_ptr<Management::FaultAnalysisService::InvokeQuorumLossProgress> &&progress);

        std::shared_ptr<Management::FaultAnalysisService::InvokeQuorumLossProgress> const & GetProgress();

    private:
        std::shared_ptr<Management::FaultAnalysisService::InvokeQuorumLossProgress> invokeQuorumLossProgress_;
    };
}
