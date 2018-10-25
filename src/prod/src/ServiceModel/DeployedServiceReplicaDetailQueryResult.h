// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class DeployedServiceReplicaDetailQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(DeployedServiceReplicaDetailQueryResult)
    
    public:
        DeployedServiceReplicaDetailQueryResult();

        // Stateless
        DeployedServiceReplicaDetailQueryResult(
            std::wstring const & serviceName,
            Common::Guid const & partitionId,
            int64 instanceId,
            FABRIC_QUERY_SERVICE_OPERATION_NAME currentServiceOperation,
            Common::DateTime currentServiceOperationStartTime,
            std::vector<ServiceModel::LoadMetricReport> && reportedLoad);

        // Stateful
        DeployedServiceReplicaDetailQueryResult(
            std::wstring const & serviceName,
            Common::Guid const & partitionId,
            int64 replicaId,
            FABRIC_QUERY_SERVICE_OPERATION_NAME currentServiceOperation,
            Common::DateTime currentServiceOperationStartTime,
            FABRIC_QUERY_REPLICATOR_OPERATION_NAME currentReplicatorOperation,
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus,
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus,
            std::vector<ServiceModel::LoadMetricReport> && reportedLoad);

        DeployedServiceReplicaDetailQueryResult(DeployedServiceReplicaDetailQueryResult && other) = default;
        DeployedServiceReplicaDetailQueryResult & operator=(DeployedServiceReplicaDetailQueryResult&& other) = default;

        __declspec(property(get=get_IsInvalid)) bool IsInvalid;
        bool get_IsInvalid() const { return kind_ == FABRIC_SERVICE_KIND_INVALID; }

        __declspec(property(get = get_ReplicaStatusQueryResult)) ReplicaStatusQueryResultSPtr const & ReplicaStatusQueryResult;
        ReplicaStatusQueryResultSPtr const & get_ReplicaStatusQueryResult() const { return replicaStatus_; }

        __declspec(property(get = get_DeployedServiceReplicaQueryResult)) DeployedServiceReplicaQueryResultSPtr DeployedServiceReplicaQueryResult;
        DeployedServiceReplicaQueryResultSPtr get_DeployedServiceReplicaQueryResult() const { return deployedServiceReplicaQueryResult_; }

        void SetDeployedServiceReplicaQueryResult(ServiceModel::DeployedServiceReplicaQueryResultSPtr && result);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM & publicResult);

        void SetReplicatorStatusQueryResult(ReplicatorStatusQueryResultSPtr && result);
        void SetReplicaStatusQueryResult(ReplicaStatusQueryResultSPtr && result);

        void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_14(
            kind_, 
            serviceName_, 
            partitionId_, 
            replicaId_, 
            instanceId_, 
            currentServiceOperation_, 
            currentServiceOperationStartTime_, 
            currentReplicatorOperation_, 
            reportedLoad_, 
            readStatus_, 
            writeStatus_, 
            replicatorStatus_, 
            replicaStatus_,
            deployedServiceReplicaQueryResult_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::ServiceKind, kind_)
            SERIALIZABLE_PROPERTY(Constants::ServiceName, serviceName_)
            SERIALIZABLE_PROPERTY(Constants::PartitionId, partitionId_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::CurrentServiceOperation, currentServiceOperation_)
            SERIALIZABLE_PROPERTY(Constants::CurrentServiceOperationStartTimeUtc, currentServiceOperationStartTime_)
            SERIALIZABLE_PROPERTY(Constants::ReportedLoad, reportedLoad_)
            SERIALIZABLE_PROPERTY_IF(Constants::InstanceId, instanceId_, kind_ == FABRIC_SERVICE_KIND_STATELESS)
            SERIALIZABLE_PROPERTY_IF(Constants::ReplicaId, replicaId_, kind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_ENUM_IF(Constants::CurrentReplicatorOperation, currentReplicatorOperation_, kind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_ENUM_IF(Constants::ReadStatus, readStatus_, kind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_ENUM_IF(Constants::WriteStatus, writeStatus_, kind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(Constants::ReplicatorStatus, replicatorStatus_, replicatorStatus_ != nullptr)
            SERIALIZABLE_PROPERTY_IF(Constants::ReplicaStatus, replicaStatus_, replicaStatus_ != nullptr)
            SERIALIZABLE_PROPERTY_IF(Constants::DeployedServiceReplica, deployedServiceReplicaQueryResult_, kind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(Constants::DeployedServiceReplicaInstance, deployedServiceReplicaQueryResult_, kind_ == FABRIC_SERVICE_KIND_STATELESS)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        FABRIC_SERVICE_KIND kind_;

        std::wstring serviceName_;
        Common::Guid partitionId_;

        FABRIC_QUERY_SERVICE_OPERATION_NAME currentServiceOperation_;
        Common::DateTime currentServiceOperationStartTime_;

        std::vector<ServiceModel::LoadMetricReport> reportedLoad_;

        int64 instanceId_;

        int64 replicaId_;

        FABRIC_QUERY_REPLICATOR_OPERATION_NAME currentReplicatorOperation_;

        FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus_;
        FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus_;

        ReplicatorStatusQueryResultSPtr replicatorStatus_;
        ReplicaStatusQueryResultSPtr replicaStatus_;

        DeployedServiceReplicaQueryResultSPtr deployedServiceReplicaQueryResult_;
    };
}
