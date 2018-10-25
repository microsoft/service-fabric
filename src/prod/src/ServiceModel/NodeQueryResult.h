// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NodeQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
        DENY_COPY(NodeQueryResult)

    public:
        NodeQueryResult();

        NodeQueryResult(
            std::wstring const & nodeName,
            std::wstring const & ipAddressOrFQDN,
            std::wstring const & nodeType,
            std::wstring const & codeVersion,
            std::wstring const & configVersion,
            FABRIC_QUERY_NODE_STATUS nodeStatus,
            int64 nodeUpTimeInSeconds,
            int64 nodeDownTimeInSeconds,
            Common::DateTime nodeUpAt,
            Common::DateTime nodeDownAt,
            bool isSeedNode,
            std::wstring const & upgradeDomain,
            std::wstring const & faultDomain,
            Federation::NodeId const & nodeId,
            uint64 instanceId,
            NodeDeactivationQueryResult && nodeDeactivationInfo,
            unsigned short httpGatewayPort,
            ULONG clusterConnectionPort = 0,
            bool isStopped = false);

        NodeQueryResult(
            std::wstring && nodeName,
            std::wstring && ipAddressOrFQDN,
            std::wstring && nodeType,
            bool isSeedNode,
            std::wstring && upgradeDomain,
            std::wstring && faultDomain,
            Federation::NodeId const & nodeId);

        NodeQueryResult(
            std::wstring const & nodeName,
            std::wstring const & ipAddressOrFQDN,
            std::wstring const & nodeType,
            bool isSeedNode,
            std::wstring const & upgradeDomain,
            std::wstring const & faultDomain,
            Federation::NodeId const & nodeId);

        NodeQueryResult(NodeQueryResult && other) = default;
        NodeQueryResult & operator = (NodeQueryResult && other) = default;

        virtual ~NodeQueryResult();

        __declspec(property(get=get_NodeName, put=put_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }
        void put_NodeName(std::wstring const& name) { nodeName_ = name; }

        __declspec(property(get=get_HealthState,put=put_HealthState)) FABRIC_HEALTH_STATE HealthState;
        FABRIC_HEALTH_STATE get_HealthState() const { return healthState_; }
        void put_HealthState(FABRIC_HEALTH_STATE healthState) { healthState_ = healthState; }

        __declspec(property(get=get_NodeId)) Federation::NodeId const & NodeId;
        Federation::NodeId const & get_NodeId() const { return nodeId_; }

        __declspec(property(get=get_InstanceId)) uint64 InstanceId;
        uint64 get_InstanceId() const { return instanceId_; }

        __declspec(property(get=get_IpAddressOrFQDN)) std::wstring const & IPAddressOrFQDN;
        std::wstring const & get_IpAddressOrFQDN() const { return ipAddressOrFQDN_; }

        __declspec(property(get=get_ClusterConnectionPort)) ULONG ClusterConnectionPort;
        ULONG get_ClusterConnectionPort() const { return clusterConnectionPort_; }

        __declspec (property(get = get_httpGatewayPort)) unsigned short HttpGatewayPort;
        unsigned short get_httpGatewayPort() const { return httpGatewayPort_; }

        __declspec(property(get=get_NodeStatus)) FABRIC_QUERY_NODE_STATUS NodeStatus;
        FABRIC_QUERY_NODE_STATUS get_NodeStatus() const { return nodeStatus_; }

        __declspec(property(get = get_IsStopped, put = put_IsStopped)) bool IsStopped;
        bool get_IsStopped() const { return isStopped_; }
        void put_IsStopped(bool isStopped) { isStopped_ = isStopped; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_NODE_QUERY_RESULT_ITEM & publicNodeQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        Common::ErrorCode FromPublicApi(__in FABRIC_NODE_QUERY_RESULT_ITEM const& publicNodeQueryResult);

        //
        // IPageContinuationToken methods
        //
        std::wstring CreateContinuationToken() const override;

        FABRIC_FIELDS_21(
            nodeName_,
            nodeId_,
            ipAddressOrFQDN_,
            nodeType_,
            codeVersion_,
            configVersion_,
            nodeStatus_,
            nodeUpTimeInSeconds_,
            healthState_,
            isSeedNode_,
            upgradeDomain_,
            faultDomain_,
            nodeId_,
            instanceId_,
            clusterConnectionPort_,
            nodeDeactivationInfo_,
            isStopped_,
            nodeDownTimeInSeconds_,
            nodeUpAt_,
            nodeDownAt_,
            httpGatewayPort_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, nodeName_)
            SERIALIZABLE_PROPERTY(Constants::IpAddressOrFQDN, ipAddressOrFQDN_)
            SERIALIZABLE_PROPERTY(Constants::Type, nodeType_)
            SERIALIZABLE_PROPERTY(Constants::CodeVersion, codeVersion_)
            SERIALIZABLE_PROPERTY(Constants::ConfigVersion, configVersion_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::NodeStatus, nodeStatus_)
            SERIALIZABLE_PROPERTY(Constants::NodeUpTimeInSeconds, nodeUpTimeInSeconds_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
            SERIALIZABLE_PROPERTY(Constants::IsSeedNode, isSeedNode_)
            SERIALIZABLE_PROPERTY(Constants::UpgradeDomain, upgradeDomain_)
            SERIALIZABLE_PROPERTY(Constants::FaultDomain, faultDomain_)
            SERIALIZABLE_PROPERTY(Constants::Id, nodeId_)
            SERIALIZABLE_PROPERTY(Constants::InstanceId, instanceId_)
            SERIALIZABLE_PROPERTY(Constants::NodeDeactivationInfo, nodeDeactivationInfo_)
            SERIALIZABLE_PROPERTY(Constants::IsStopped, isStopped_)
            SERIALIZABLE_PROPERTY(Constants::NodeDownTimeInSeconds, nodeDownTimeInSeconds_)
            SERIALIZABLE_PROPERTY(Constants::NodeUpAt, nodeUpAt_)
            SERIALIZABLE_PROPERTY(Constants::NodeDownAt, nodeDownAt_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(ipAddressOrFQDN_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeType_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(codeVersion_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(configVersion_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(nodeStatus_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(healthState_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(upgradeDomain_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(faultDomain_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeId_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeDeactivationInfo_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(isStopped_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeUpAt_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeDownAt_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring nodeName_;
        std::wstring ipAddressOrFQDN_;
        std::wstring nodeType_;
        std::wstring codeVersion_;
        std::wstring configVersion_;
        FABRIC_QUERY_NODE_STATUS nodeStatus_;
        int64 nodeUpTimeInSeconds_;
        int64 nodeDownTimeInSeconds_;
        FABRIC_HEALTH_STATE healthState_;
        bool isSeedNode_;
        std::wstring upgradeDomain_;
        std::wstring faultDomain_;
        Federation::NodeId nodeId_;
        uint64 instanceId_;
        ULONG clusterConnectionPort_;
        NodeDeactivationQueryResult nodeDeactivationInfo_;
        bool isStopped_;
        Common::DateTime nodeUpAt_;
        Common::DateTime nodeDownAt_;
        unsigned short httpGatewayPort_ = 0;
    };

    // Used to serialize results in REST
    QUERY_JSON_LIST(NodeList, NodeQueryResult)
}
