// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace UpgradeOrchestrationService
    {
        class GetClusterConfigurationMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            GetClusterConfigurationMessageBody() : apiVersion_() { }

            GetClusterConfigurationMessageBody(std::wstring const & apiVersion) : apiVersion_(apiVersion) { }

            __declspec(property(get = get_ApiVersion)) std::wstring const & ApiVersion;

            std::wstring const & get_ApiVersion() const { return apiVersion_; }

            FABRIC_FIELDS_01(apiVersion_);

        private:
            std::wstring apiVersion_;
        };
    }
}
