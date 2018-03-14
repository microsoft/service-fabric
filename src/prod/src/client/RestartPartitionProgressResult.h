// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class RestartPartitionProgressResult
        : public Api::IRestartPartitionProgressResult
        , public Common::ComponentRoot
    {
        DENY_COPY(RestartPartitionProgressResult);

    public:
        RestartPartitionProgressResult(
            __in std::shared_ptr<Management::FaultAnalysisService::RestartPartitionProgress> &&progress);

        std::shared_ptr<Management::FaultAnalysisService::RestartPartitionProgress> const & GetProgress();

    private:
        std::shared_ptr<Management::FaultAnalysisService::RestartPartitionProgress> restartPartitionProgress_;
    };
}
