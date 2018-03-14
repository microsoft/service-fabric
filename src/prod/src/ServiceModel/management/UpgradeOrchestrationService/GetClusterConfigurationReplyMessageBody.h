// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace UpgradeOrchestrationService
    {
        class GetClusterConfigurationReplyMessageBody : public Serialization::FabricSerializable
        {
        public:
            GetClusterConfigurationReplyMessageBody() : clusterConfiguration_() { }

            GetClusterConfigurationReplyMessageBody(std::wstring && clusterConfiguration) : clusterConfiguration_(std::move(clusterConfiguration)) { }

            __declspec(property(get = get_ClusterConfiguration)) std::wstring & ClusterConfiguration;

            std::wstring & get_ClusterConfiguration() { return clusterConfiguration_; }

            FABRIC_FIELDS_01(clusterConfiguration_);

        private:
            std::wstring clusterConfiguration_;
        };
    }
}
