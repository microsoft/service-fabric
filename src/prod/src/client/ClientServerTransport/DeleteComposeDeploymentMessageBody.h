// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class DeleteComposeDeploymentMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            DeleteComposeDeploymentMessageBody() = default;

            DeleteComposeDeploymentMessageBody(
                std::wstring const & deploymentName,
                Common::NamingUri const & applicationName)
                : deploymentName_(deploymentName)
                , applicationName_(applicationName)
            {
            }

            __declspec(property(get = get_ApplicationName)) Common::NamingUri const & ApplicationName;
            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }

            __declspec(property(get = get_DeploymentName)) std::wstring const & DeploymentName;
            std::wstring const & get_DeploymentName() const { return deploymentName_; }

            FABRIC_FIELDS_02(applicationName_, deploymentName_);

        private:
            std::wstring deploymentName_;
            Common::NamingUri applicationName_;
        };
    }
}
