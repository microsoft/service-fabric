// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class RemoteReplicatorAcknowledgementDetail
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        RemoteReplicatorAcknowledgementDetail();

        RemoteReplicatorAcknowledgementDetail(
            int64 avgReplicationReceiveAckDuration,
            int64 avgReplicationApplyAckDuration,
            FABRIC_SEQUENCE_NUMBER notReceivedReplicationCount,
            FABRIC_SEQUENCE_NUMBER receivedAndNotAppliedReplicationCount);

        RemoteReplicatorAcknowledgementDetail(RemoteReplicatorAcknowledgementDetail && other);
                RemoteReplicatorAcknowledgementDetail & operator=(RemoteReplicatorAcknowledgementDetail && other);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_DETAIL & publicResult) const ;

        Common::ErrorCode FromPublicApi(
            __in FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_DETAIL & publicResult) ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_04(
            avgReceiveAckDuration_,
            avgApplyAckDuration_,
            notReceivedCount_,
            receivedAndNotAppliedCount_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::AverageReceiveDuration, avgReceiveAckDuration_)
            SERIALIZABLE_PROPERTY(Constants::AverageApplyDuration, avgApplyAckDuration_)
            SERIALIZABLE_PROPERTY(Constants::NotReceivedCount, notReceivedCount_)
            SERIALIZABLE_PROPERTY(Constants::ReceivedAndNotAppliedCount, receivedAndNotAppliedCount_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        int64 avgReceiveAckDuration_;
        int64 avgApplyAckDuration_;
        FABRIC_SEQUENCE_NUMBER notReceivedCount_;
        FABRIC_SEQUENCE_NUMBER receivedAndNotAppliedCount_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::RemoteReplicatorAcknowledgementDetail);
