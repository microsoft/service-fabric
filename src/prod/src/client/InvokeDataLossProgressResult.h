// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class InvokeDataLossProgressResult
        : public Api::IInvokeDataLossProgressResult
        , public Common::ComponentRoot
    {
        DENY_COPY(InvokeDataLossProgressResult);

    public:
        InvokeDataLossProgressResult(
            __in std::shared_ptr<Management::FaultAnalysisService::InvokeDataLossProgress> &&progress);

        std::shared_ptr<Management::FaultAnalysisService::InvokeDataLossProgress> const & GetProgress();

    private:
        std::shared_ptr<Management::FaultAnalysisService::InvokeDataLossProgress> invokeDataLossProgress_;
    };
}
