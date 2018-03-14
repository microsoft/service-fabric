// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class SecondaryReplicatorStatusQueryResult
        : public ReplicatorStatusQueryResult
    {
    private:
        SecondaryReplicatorStatusQueryResult(
            ReplicatorQueueStatusSPtr && replicationQueueStatus,
            Common::DateTime && lastReplicationOperationReceivedTime,
            bool isInBuild,
            ReplicatorQueueStatusSPtr && copyQueueStatus,
            Common::DateTime && lastCopyOperationReceivedTime,
            Common::DateTime && lastAcknowledgementSentTime);

    public:
        SecondaryReplicatorStatusQueryResult();

        SecondaryReplicatorStatusQueryResult(SecondaryReplicatorStatusQueryResult && other);

        static ReplicatorStatusQueryResultSPtr Create(
            ReplicatorQueueStatusSPtr && replicationQueueStatus,
            Common::DateTime && lastReplicationOperationReceivedTime,
            bool isInBuild,
            ReplicatorQueueStatusSPtr && copyQueueStatus,
            Common::DateTime && lastCopyOperationReceivedTime,
            Common::DateTime && lastAcknowledgementSentTime);
         
        SecondaryReplicatorStatusQueryResult const & operator = (SecondaryReplicatorStatusQueryResult && other);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_REPLICATOR_STATUS_QUERY_RESULT & publicResult) const ;

        Common::ErrorCode FromPublicApi(
            __in FABRIC_REPLICATOR_STATUS_QUERY_RESULT const & publicResult) ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_07(
            role_,
            replicationQueueStatus_,
            lastReplicationOperationReceivedTime_,
            isInBuild_, 
            copyQueueStatus_, 
            lastCopyOperationReceivedTime_, 
            lastAcknowledgementSentTime_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::ReplicationQueueStatus, replicationQueueStatus_)
            SERIALIZABLE_PROPERTY(Constants::LastReplicationOperationReceivedTimeUtc, lastReplicationOperationReceivedTime_)
            SERIALIZABLE_PROPERTY(Constants::IsInBuild, isInBuild_)
            SERIALIZABLE_PROPERTY(Constants::CopyQueueStatus, copyQueueStatus_)
            SERIALIZABLE_PROPERTY(Constants::LastCopyOperationReceivedTimeUtc, lastCopyOperationReceivedTime_)
            SERIALIZABLE_PROPERTY(Constants::LastAcknowledgementSentTimeUtc, lastAcknowledgementSentTime_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        ReplicatorQueueStatusSPtr replicationQueueStatus_;
        Common::DateTime lastReplicationOperationReceivedTime_;
        
        bool isInBuild_;
        ReplicatorQueueStatusSPtr copyQueueStatus_;
        Common::DateTime lastCopyOperationReceivedTime_;

        Common::DateTime lastAcknowledgementSentTime_;
    };
}
