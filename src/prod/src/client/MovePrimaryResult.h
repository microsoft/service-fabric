// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class MovePrimaryResult
        : public Api::IMovePrimaryResult
        , public Common::ComponentRoot
    {
        DENY_COPY(MovePrimaryResult);

    public:
        MovePrimaryResult(
            __in Management::FaultAnalysisService::MovePrimaryStatus && status);

        std::shared_ptr<Management::FaultAnalysisService::PrimaryMoveResult> const & GetMovePrimaryResult() override;

    private:
        Management::FaultAnalysisService::MovePrimaryStatus status_;
    };
}
