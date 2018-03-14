// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class MoveSecondaryResult
        : public Api::IMoveSecondaryResult
        , public Common::ComponentRoot
    {
        DENY_COPY(MoveSecondaryResult);

    public:
        MoveSecondaryResult(
            __in Management::FaultAnalysisService::MoveSecondaryStatus && status);

        std::shared_ptr<Management::FaultAnalysisService::SecondaryMoveResult> const & GetMoveSecondaryResult() override;

    private:
        Management::FaultAnalysisService::MoveSecondaryStatus status_;
    };
}
