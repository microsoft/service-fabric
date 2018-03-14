// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class StartNodeDescriptionUsingNodeName
            : public Serialization::FabricSerializable
        {
        public:

            StartNodeDescriptionUsingNodeName();
            StartNodeDescriptionUsingNodeName(StartNodeDescriptionUsingNodeName const &);
            StartNodeDescriptionUsingNodeName(StartNodeDescriptionUsingNodeName &&);

            __declspec(property(get = get_NodeName)) std::wstring const & NodeName;
            __declspec(property(get = get_NodeInstanceId)) uint64 NodeInstanceId;
            __declspec(property(get = get_IPAddressOrFQDN)) std::wstring const & IPAddressOrFQDN;
            __declspec(property(get = get_ClusterConnectionPort)) ULONG const ClusterConnectionPort;

            std::wstring const & get_NodeName() const { return nodeName_; }
            uint64 get_NodeInstanceId() const { return instanceId_; }
            std::wstring const & get_IPAddressOrFQDN() const { return ipAddressOrFQDN_; }
            ULONG get_ClusterConnectionPort() const { return clusterConnectionPort_; }

            Common::ErrorCode FromPublicApi(FABRIC_START_NODE_DESCRIPTION_USING_NODE_NAME const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_START_NODE_DESCRIPTION_USING_NODE_NAME &) const;

            FABRIC_FIELDS_04(
                nodeName_,
                instanceId_,
                ipAddressOrFQDN_,
                clusterConnectionPort_);

        private:

            std::wstring nodeName_;
            uint64 instanceId_;
            std::wstring ipAddressOrFQDN_;
            ULONG clusterConnectionPort_;
        };
    }
}
