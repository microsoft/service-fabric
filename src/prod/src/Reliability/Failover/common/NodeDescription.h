// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NodeDescription : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_ASSIGNMENT(NodeDescription)
    
    public:
        NodeDescription();

        NodeDescription(Federation::NodeId const& nodeId);

        NodeDescription(
            Common::FabricVersionInstance const& versionInstance,
            std::wstring const& nodeUpgradeDomainId,
            std::vector<Common::Uri> const& nodeFaultDomainIds,
            std::map<std::wstring, std::wstring> const& nodePropertyMap,
            std::map<std::wstring, uint> const& nodeCapacityRatioMap,
            std::map<std::wstring, uint> const& nodeCapacityMap,
            std::wstring const& nodeName,
            std::wstring const& nodeType,
            std::wstring const& ipAddressOrFQDN,
            ULONG clusterConnectionPort,
            unsigned short httpGatewayPort);

        NodeDescription(NodeDescription const& other) = default;

        NodeDescription(NodeDescription && other) = default;

        void InitializeNodeName(Federation::NodeId const& nodeId);

        NodeDescription & operator=(NodeDescription && other) = default;

        __declspec (property(get=get_VersionInstance)) Common::FabricVersionInstance const& VersionInstance;
        Common::FabricVersionInstance const& get_VersionInstance() const { return versionInstance_; }

        __declspec (property(get=get_NodeUpgradeDomainId)) std::wstring const& NodeUpgradeDomainId;
        std::wstring const& get_NodeUpgradeDomainId() const { return nodeUpgradeDomainId_; }

        __declspec (property(get=get_NodeFaultDomainIds)) std::vector<Common::Uri> const& NodeFaultDomainIds;
        std::vector<Common::Uri> const& get_NodeFaultDomainIds() const { return nodeFaultDomainIds_; }

        __declspec (property(get=get_NodePropertyMap)) std::map<std::wstring, std::wstring> const& NodePropertyMap;
        std::map<std::wstring, std::wstring> const& get_NodePropertyMap() const { return nodePropertyMap_; }

        __declspec (property(get=get_NodeCapacityRatioMap)) std::map<std::wstring, uint> const& NodeCapacityRatioMap;
        std::map<std::wstring, uint> const& get_NodeCapacityRatioMap() const { return nodeCapacityRatioMap_; }

        __declspec (property(get=get_NodeCapacityMap)) std::map<std::wstring, uint> const& NodeCapacityMap;
        std::map<std::wstring, uint> const& get_NodeCapacityMap() const { return nodeCapacityMap_; }

        __declspec (property(get=get_NodeName)) std::wstring const& NodeName;
        std::wstring const& get_NodeName() const { return nodeName_; }

        __declspec (property(get=get_NodeType)) std::wstring const& NodeType;
        std::wstring const& get_NodeType() const { return nodeType_; }

        __declspec (property(get=get_ipAddressOrFQDN)) std::wstring const& IpAddressOrFQDN;
        std::wstring const& get_ipAddressOrFQDN() const { return ipAddressOrFQDN_; }

        __declspec (property(get=get_clusterConnectionPort)) ULONG ClusterConnectionPort;
        ULONG get_clusterConnectionPort() const { return clusterConnectionPort_; }

        __declspec (property(get = get_httpGatewayPort)) unsigned short HttpGatewayPort;
        unsigned short get_httpGatewayPort() const { return httpGatewayPort_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        FABRIC_FIELDS_11(nodeUpgradeDomainId_, nodeFaultDomainIds_, nodePropertyMap_, nodeCapacityRatioMap_, nodeCapacityMap_, versionInstance_, nodeName_, nodeType_, ipAddressOrFQDN_, clusterConnectionPort_, httpGatewayPort_);

    private:

        Common::FabricVersionInstance versionInstance_;
        std::wstring nodeUpgradeDomainId_;
        std::vector<Common::Uri> nodeFaultDomainIds_;
        std::map<std::wstring, std::wstring> nodePropertyMap_;
        std::map<std::wstring, uint> nodeCapacityRatioMap_;
        std::map<std::wstring, uint> nodeCapacityMap_;
        std::wstring nodeName_;
        std::wstring nodeType_;
        std::wstring ipAddressOrFQDN_;
        ULONG clusterConnectionPort_;
        unsigned short httpGatewayPort_ = 0;
    };
}

DEFINE_USER_MAP_UTILITY(std::wstring, uint);
