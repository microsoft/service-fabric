// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class DeployedServiceReplicaQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        DeployedServiceReplicaQueryResult();
        DeployedServiceReplicaQueryResult(
            std::wstring const & serviceName,
            std::wstring const & serviceTypeName,
            std::wstring const & serviceManifestName,
            std::shared_ptr<std::wstring> const & servicePackageActivationIdFilterSPtr,
            std::wstring const & codePackageName,
            Common::Guid const & partitionId,
            int64 replicaId,
            FABRIC_REPLICA_ROLE replicaRole, 
            FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus,
            std::wstring const & address,
            int64 const & hostProcessId,
            ReconfigurationInformation const & reconfigurationInformation);

        DeployedServiceReplicaQueryResult(
            std::wstring const & serviceName,
            std::wstring const & serviceTypeName,
            std::wstring const & serviceManifestName,
            std::shared_ptr<std::wstring> const & servicePackageActivationIdFilterSPtr,
            std::wstring const & codePackageName,
            Common::Guid const & partitionId,
            int64 instanceId,
            FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus,
            std::wstring const & address,
            int64 hostProcessId);

        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() const { return serviceName_; }

        __declspec(property(get=get_ServiceTypeName)) std::wstring const & ServiceTypeName;
        std::wstring const & get_ServiceTypeName() const { return serviceTypeName_; }

        __declspec(property(get=get_ServiceManifestVersion)) std::wstring const & ServiceManifestVersion;
        std::wstring const & get_ServiceManifestVersion() const { return serviceManifestVersion_; }

        __declspec(property(get=get_ServiceManifestName)) std::wstring const & ServiceManifestName;
        std::wstring const & get_ServiceManifestName() const { return serviceManifestName_; }

        __declspec(property(get = get_ServicePackageActivationIdSPtr)) std::shared_ptr<std::wstring> const & servicePackageActivationIdFilterSPtr;
        std::shared_ptr<std::wstring> const & get_ServicePackageActivationIdSPtr() const { return servicePackageActivationIdFilterSPtr_; }

        __declspec(property(get=get_CodePackageName)) std::wstring const & CodePackageName;
        std::wstring const & get_CodePackageName() const { return codePackageName_; }

        __declspec(property(get=get_InstanceId)) int64 InstanceId;
        int64 get_InstanceId() const { return instanceId_; }

        __declspec(property(get=get_ReplicaId)) int64 ReplicaId;
        int64 get_ReplicaId() const { return replicaId_; }

        __declspec(property(get=get_PartitionId)) Common::Guid PartitionId;
        Common::Guid get_PartitionId() const { return partitionId_; }

        __declspec(property(get=get_ReplicaStatus)) FABRIC_QUERY_SERVICE_REPLICA_STATUS ReplicaStatus;
        FABRIC_QUERY_SERVICE_REPLICA_STATUS get_ReplicaStatus() const { return replicaStatus_; }

        __declspec(property(get=get_Address)) std::wstring const & Address;
        std::wstring const & get_Address() const { return address_; }

        __declspec(property(get=get_ReplicaRole)) FABRIC_REPLICA_ROLE ReplicaRole;
        FABRIC_REPLICA_ROLE get_ReplicaRole() const { return replicaRole_; }

        __declspec(property(get = get_HostProcessId)) int64 HostProcessId;
        int64 get_HostProcessId() const { return hostProcessId_; }

        __declspec(property(get = get_ReconfigurationInformation)) ReconfigurationInformation ReconfigInformation;
        ReconfigurationInformation get_ReconfigurationInformation() const { return reconfigurationInformation_; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM & publicDeployedServiceManifestQueryResult) const ;

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM & publicDeployedStatefulServiceManifestQueryResult) const;

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM & publicDeployedStatelessServiceManifestQueryResult) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        Common::ErrorCode FromPublicApi(__in FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM const & publicDeployedServiceManifestQueryResult);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::ServiceKind, serviceResultKind_)
            SERIALIZABLE_PROPERTY(Constants::ServiceName, serviceName_)
            SERIALIZABLE_PROPERTY(Constants::ServiceTypeName, serviceTypeName_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestVersion, serviceManifestVersion_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestName, serviceManifestName_)
            SERIALIZABLE_PROPERTY(Constants::CodePackageName, codePackageName_)
            SERIALIZABLE_PROPERTY(Constants::PartitionId, partitionId_)
            SERIALIZABLE_PROPERTY_IF(Constants::ReplicaId, replicaId_, serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_ENUM_IF(Constants::ReplicaRole, replicaRole_, serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(Constants::InstanceId, instanceId_, serviceResultKind_ == FABRIC_SERVICE_KIND_STATELESS)
            SERIALIZABLE_PROPERTY_ENUM(Constants::ReplicaStatus, replicaStatus_)
            SERIALIZABLE_PROPERTY(Constants::Address, address_)
            SERIALIZABLE_PROPERTY(Constants::ServicePackageActivationId, servicePackageActivationIdFilterSPtr_)
            SERIALIZABLE_PROPERTY(Constants::HostProcessId, hostProcessId_)
            SERIALIZABLE_PROPERTY_IF(Constants::ReconfigurationInformation, reconfigurationInformation_, serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_15(
            serviceResultKind_,
            serviceName_,
            serviceTypeName_,
            serviceManifestVersion_,
            codePackageName_,
            partitionId_,
            replicaId_,
            instanceId_,
            replicaRole_,
            replicaStatus_,
            address_,
            serviceManifestName_,
            servicePackageActivationIdFilterSPtr_,
            hostProcessId_,
            reconfigurationInformation_);

    private:
        FABRIC_SERVICE_KIND serviceResultKind_;

        std::wstring serviceName_;
        std::wstring serviceTypeName_;
        std::wstring serviceManifestVersion_;
        std::wstring codePackageName_;
        Common::Guid partitionId_;
        int64 replicaId_;
        int64 instanceId_;
        FABRIC_REPLICA_ROLE replicaRole_; 
        FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus_;
        std::wstring address_;
        std::wstring serviceManifestName_;
        std::shared_ptr<std::wstring> servicePackageActivationIdFilterSPtr_;
        int64 hostProcessId_;
        ReconfigurationInformation reconfigurationInformation_;
    };
}
