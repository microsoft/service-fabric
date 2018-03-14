// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class RestartDeployedCodePackageResult
        : public Api::IRestartDeployedCodePackageResult
        , public Common::ComponentRoot
    {
        DENY_COPY(RestartDeployedCodePackageResult);

    public:
        RestartDeployedCodePackageResult(
            __in Management::FaultAnalysisService::RestartDeployedCodePackageStatus && status);

        std::shared_ptr<Management::FaultAnalysisService::DeployedCodePackageResult> const & GetRestartDeployedCodePackageResult() override;

    private:
        Management::FaultAnalysisService::RestartDeployedCodePackageStatus status_;
    };
}
