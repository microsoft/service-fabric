// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class RollbackComposeDeploymentMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            RollbackComposeDeploymentMessageBody() = default;

            RollbackComposeDeploymentMessageBody(
                std::wstring const & deploymentName)
                : ClientServerMessageBody(), deploymentName_(deploymentName)
            {
            }

            __declspec(property(get = get_DeploymentName)) std::wstring const & DeploymentName;
            std::wstring const & get_DeploymentName() const { return deploymentName_; }

            FABRIC_FIELDS_01(deploymentName_);

        private:
            std::wstring deploymentName_;
        };
    }
}
