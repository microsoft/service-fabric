// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class RemoteReplicatorAcknowledgementStatus
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        RemoteReplicatorAcknowledgementStatus();

        RemoteReplicatorAcknowledgementStatus(
            RemoteReplicatorAcknowledgementDetail && replicationStreamAcknowledgementDetail,
            RemoteReplicatorAcknowledgementDetail && copyStreamAcknowledgementDetail);

        RemoteReplicatorAcknowledgementStatus(RemoteReplicatorAcknowledgementStatus && other);
        
        RemoteReplicatorAcknowledgementStatus & operator=(RemoteReplicatorAcknowledgementStatus && other);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_STATUS & publicResult) const ;

        Common::ErrorCode FromPublicApi(
            __in FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_STATUS & publicResult) ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const ;
        std::wstring ToString() const;

        FABRIC_FIELDS_02(
            replicationStreamAcknowledgementDetail_,
            copyStreamAcknowledgementDetail_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ReplicationStreamAcknowledgementDetail, replicationStreamAcknowledgementDetail_)
            SERIALIZABLE_PROPERTY(Constants::CopyStreamAcknowledgementDetail, copyStreamAcknowledgementDetail_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        RemoteReplicatorAcknowledgementDetail replicationStreamAcknowledgementDetail_;
        RemoteReplicatorAcknowledgementDetail copyStreamAcknowledgementDetail_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::RemoteReplicatorAcknowledgementStatus);
