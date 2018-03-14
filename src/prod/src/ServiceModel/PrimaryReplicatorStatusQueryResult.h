// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class PrimaryReplicatorStatusQueryResult
        : public ReplicatorStatusQueryResult
    {
    private:
        PrimaryReplicatorStatusQueryResult(
            std::shared_ptr<ReplicatorQueueStatus> && replicationQueueStatus,
            std::vector<RemoteReplicatorStatus> && remoteReplicators);

    public:
        static ReplicatorStatusQueryResultSPtr Create(
            std::shared_ptr<ReplicatorQueueStatus> && replicationQueueStatus,
            std::vector<RemoteReplicatorStatus> && remoteReplicators);

        PrimaryReplicatorStatusQueryResult(PrimaryReplicatorStatusQueryResult && other);
        
        PrimaryReplicatorStatusQueryResult();

        PrimaryReplicatorStatusQueryResult & operator=(PrimaryReplicatorStatusQueryResult && other);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_REPLICATOR_STATUS_QUERY_RESULT & publicResult) const ;

        Common::ErrorCode FromPublicApi(
            __in FABRIC_REPLICATOR_STATUS_QUERY_RESULT const & publicResult) ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_03(
            role_,
            replicationQueueStatus_,
            remoteReplicators_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::ReplicationQueueStatus, replicationQueueStatus_)
            SERIALIZABLE_PROPERTY(Constants::RemoteReplicators, remoteReplicators_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        ReplicatorQueueStatusSPtr replicationQueueStatus_;
        std::vector<RemoteReplicatorStatus> remoteReplicators_;
    };
}
