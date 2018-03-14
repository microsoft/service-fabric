// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class RestartDeployedCodePackageStatus : public Serialization::FabricSerializable
        {
        public:
            RestartDeployedCodePackageStatus();

            RestartDeployedCodePackageStatus(RestartDeployedCodePackageStatus &&);

            RestartDeployedCodePackageStatus & operator=(RestartDeployedCodePackageStatus &&);

            RestartDeployedCodePackageStatus(
                std::shared_ptr<Management::FaultAnalysisService::DeployedCodePackageResult> && deployedCodePackageResult);

            __declspec(property(get = get_DeployedCodePackageResult)) std::shared_ptr<Management::FaultAnalysisService::DeployedCodePackageResult> const & ResultSPtr;
            std::shared_ptr<Management::FaultAnalysisService::DeployedCodePackageResult> const & get_DeployedCodePackageResult() const { return deployedCodePackageResultSPtr_; }

            std::shared_ptr<Management::FaultAnalysisService::DeployedCodePackageResult> const & GetRestartDeployedCodePackageResult();

            FABRIC_FIELDS_01(
                deployedCodePackageResultSPtr_);

        private:
            std::shared_ptr<Management::FaultAnalysisService::DeployedCodePackageResult> deployedCodePackageResultSPtr_;
        };
    }
}
