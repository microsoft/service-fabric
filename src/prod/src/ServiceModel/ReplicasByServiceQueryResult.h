//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ReplicaInfoResult
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DEFAULT_COPY_CONSTRUCTOR(ReplicaInfoResult)

    public:
        ReplicaInfoResult() = default;

        ReplicaInfoResult(
            Common::Guid const &partitionId,
            std::wstring const &partitionName,
            int64 replicaId,
            std::wstring nodeName)
            : PartitionId(partitionId)
            , PartitionName(partitionName)
            , ReplicaId(replicaId)
            , NodeName(nodeName)
        {
        }

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(PartitionId)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(PartitionName)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(ReplicaId)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(NodeName)
        END_DYNAMIC_SIZE_ESTIMATION()

        FABRIC_FIELDS_04(
            PartitionId,
            PartitionName,
            ReplicaId,
            NodeName)

        Common::Guid PartitionId;
        std::wstring PartitionName;
        int64 ReplicaId;
        std::wstring NodeName;
    };

    //
    // This is an intermediate result used by queries that need to know about all the replicas of a service.
    // Eg: GetReplicaResourceList query.
    //
    class ReplicasByServiceQueryResult
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
    DEFAULT_COPY_CONSTRUCTOR(ReplicasByServiceQueryResult)

    public:
        ReplicasByServiceQueryResult()
            : ServiceName()
            , ReplicaInfos()
        {
        }
        
        ReplicasByServiceQueryResult(std::wstring const & serviceName)
            : ServiceName(serviceName)
            , ReplicaInfos()
        {
        }

        ReplicasByServiceQueryResult(
            std::wstring const &serviceName,
            std::vector<ReplicaInfoResult> &&replicaInfos);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        //
        // IPageContinuationToken methods
        //
        std::wstring CreateContinuationToken() const override;

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(ServiceName)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(ReplicaInfos)
        END_DYNAMIC_SIZE_ESTIMATION()

        FABRIC_FIELDS_02(
            ServiceName,
            ReplicaInfos)

        std::wstring ServiceName;
        std::vector<ReplicaInfoResult> ReplicaInfos;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ReplicaInfoResult);
