// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServicePackageInfo  : public Serialization::FabricSerializable
    {
    public:
        ServicePackageInfo()
        {
        }

        ServicePackageInfo(
            ServiceModel::ServicePackageIdentifier const & packageId,
            ServiceModel::ServicePackageVersionInstance const & versionInstance)
            :   packageId_(packageId), versionInstance_(versionInstance)
        {
        }

        __declspec (property(get=get_PackageId)) ServiceModel::ServicePackageIdentifier const& PackageId;
        ServiceModel::ServicePackageIdentifier const& get_PackageId() const { return packageId_; }

        __declspec (property(get=get_VersionInstance)) ServiceModel::ServicePackageVersionInstance const& VersionInstance;
        ServiceModel::ServicePackageVersionInstance const& get_VersionInstance() const { return versionInstance_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
        {
            w.Write("[{0}, {1}]", packageId_, versionInstance_);
        }

        FABRIC_FIELDS_02(packageId_, versionInstance_);

    private:
        ServiceModel::ServicePackageIdentifier packageId_;
        ServiceModel::ServicePackageVersionInstance versionInstance_;
    };

    class NodeUpMessageBody : public Serialization::FabricSerializable
    {
    public:
        NodeUpMessageBody()
        {
        }

        NodeUpMessageBody(
            std::vector<ServicePackageInfo> const& packages,
            NodeDescription const& nodeDescription,
            bool anyReplicaFound)
            : nodeDescription_(nodeDescription),
              packages_(packages),
              anyReplicaFound_(anyReplicaFound)
        {
        }

        __declspec (property(get=get_Packages)) std::vector<ServicePackageInfo> const & Packages;
        std::vector<ServicePackageInfo> const & get_Packages() const { return packages_; }

        __declspec (property(get=get_VersionInstance)) Common::FabricVersionInstance const& VersionInstance;
        Common::FabricVersionInstance const& get_VersionInstance() const { return nodeDescription_.VersionInstance; }

        __declspec (property(get=get_NodePropertyMap)) std::map<std::wstring, std::wstring> const& NodePropertyMap;
        std::map<std::wstring, std::wstring> const& get_NodePropertyMap() const { return nodeDescription_.NodePropertyMap; }

        __declspec (property(get=get_NodeUpgradeDomainId)) std::wstring const& NodeUpgradeDomainId;
        std::wstring const& get_NodeUpgradeDomainId() const { return nodeDescription_.NodeUpgradeDomainId; }

        __declspec (property(get=get_NodeFaultDomainIds)) std::vector<Common::Uri> const& NodeFaultDomainIds;
        std::vector<Common::Uri> const& get_NodeFaultDomainIds() const { return nodeDescription_.NodeFaultDomainIds; }

        __declspec (property(get=get_NodeCapacityRatioMap)) std::map<std::wstring, uint> const& NodeCapacityRatioMap;
        std::map<std::wstring, uint> get_NodeCapacityRatioMap() const { return nodeDescription_.NodeCapacityRatioMap; }

        __declspec (property(get=get_NodeCapacityMap)) std::map<std::wstring, uint> const& NodeCapacityMap;
        std::map<std::wstring, uint> get_NodeCapacityMap() const { return nodeDescription_.NodeCapacityMap; }

        __declspec (property(get=get_NodeName)) std::wstring const& NodeName;
        std::wstring const& get_NodeName() const { return nodeDescription_.NodeName; }

        __declspec (property(get=get_NodeType)) std::wstring const& NodeType;
        std::wstring const& get_NodeType() const { return nodeDescription_.NodeType; }

        __declspec (property(get=get_IpAddressOrFQDN)) std::wstring const& IpAddressOrFQDN;
        std::wstring const& get_IpAddressOrFQDN() const { return nodeDescription_.IpAddressOrFQDN; }

        __declspec (property(get=get_ClusterConnectionPort)) ULONG ClusterConnectionPort;
        ULONG get_ClusterConnectionPort() const { return nodeDescription_.ClusterConnectionPort; }

        __declspec (property(get = get_httpGatewayPort)) unsigned short HttpGatewayPort;
        unsigned short get_httpGatewayPort() const { return nodeDescription_.HttpGatewayPort; }

        __declspec (property(get=get_AnyReplicaFound)) bool AnyReplicaFound;
        bool get_AnyReplicaFound() const { return anyReplicaFound_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
        {
            w.Write("{0}, {1}, {2}", nodeDescription_, packages_, anyReplicaFound_);
        }

        FABRIC_FIELDS_03(nodeDescription_, packages_, anyReplicaFound_);

    private:
        Reliability::NodeDescription nodeDescription_;
        std::vector<ServicePackageInfo> packages_;
        bool anyReplicaFound_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::ServicePackageInfo);

