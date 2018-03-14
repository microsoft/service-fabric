// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StartNodeRequestMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        StartNodeRequestMessageBody();

        StartNodeRequestMessageBody(
            std::wstring && nodeName,
            uint64 instanceId,
            std::wstring && ipAddressOrFQDN,
            ULONG clusterConnectionPort);

        StartNodeRequestMessageBody(
            std::wstring const & nodeName,
            uint64 instanceId,
            std::wstring const & ipAddressOrFQDN,
            ULONG clusterConnectionPort);

        StartNodeRequestMessageBody(StartNodeRequestMessageBody && other);

        StartNodeRequestMessageBody & operator=(StartNodeRequestMessageBody && other);

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get=get_InstanceId)) uint64  InstanceId;
        uint64 const get_InstanceId() const { return instanceId_; }

        __declspec(property(get=get_IpAddressOrFQDN)) std::wstring const & IpAddressOrFQDN;
        std::wstring const & get_IpAddressOrFQDN() const { return ipAddressOrFQDN_; }

        __declspec(property(get=get_ClusterConnectionPort)) ULONG const & ClusterConnectionPort;
        ULONG const & get_ClusterConnectionPort() const { return clusterConnectionPort_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        {
           w.Write("Node = {0}:{1} IpAddressOrFQDN = {2} ClusterConnectionPort = {3}", nodeName_, instanceId_, ipAddressOrFQDN_, clusterConnectionPort_);
        }

        FABRIC_FIELDS_04(nodeName_, instanceId_, ipAddressOrFQDN_, clusterConnectionPort_);

    private:
        std::wstring nodeName_;
        uint64 instanceId_;
        std::wstring ipAddressOrFQDN_;
        ULONG clusterConnectionPort_;
    };
}

