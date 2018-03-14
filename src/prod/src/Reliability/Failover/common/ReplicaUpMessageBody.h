// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ReplicaUpMessageBody : public Serialization::FabricSerializable
    {
    public:

        ReplicaUpMessageBody()
        {
        }

        ReplicaUpMessageBody(
            std::vector<Reliability::FailoverUnitInfo> && replicaList, 
            std::vector<Reliability::FailoverUnitInfo> && droppedReplicas,
            bool isLastReplicaUpMessage,
            bool isFromFMM = false)
            : replicaList_(std::move(replicaList)),
              droppedReplicas_(std::move(droppedReplicas)),
              isLastReplicaUpMessage_(isLastReplicaUpMessage),
              isFromFMM_(isFromFMM)
        {
        }

        __declspec (property(get=get_ReplicaList)) std::vector<Reliability::FailoverUnitInfo> const & ReplicaList;
        std::vector<Reliability::FailoverUnitInfo> const& get_ReplicaList() const { return replicaList_; }

        __declspec (property(get=get_DroppedReplicas)) std::vector<Reliability::FailoverUnitInfo> const & DroppedReplicas;
        std::vector<Reliability::FailoverUnitInfo> const& get_DroppedReplicas() const { return droppedReplicas_; }

        __declspec (property(get=get_IsLastReplicaUpMessage)) bool IsLastReplicaUpMessage;
        bool get_IsLastReplicaUpMessage() const { return isLastReplicaUpMessage_; }

        __declspec (property(get=get_IsFromFMM)) bool IsFromFMM;
        bool get_IsFromFMM() const { return isFromFMM_; }

        __declspec(property(get = get_Sender)) Reliability::FailoverManagerId Sender;
        Reliability::FailoverManagerId get_Sender() const { return Reliability::FailoverManagerId(isFromFMM_); }

        std::vector<Reliability::FailoverUnitInfo> & GetReplicaList() { return replicaList_; }
        std::vector<Reliability::FailoverUnitInfo> & GetDroppedReplicas() { return droppedReplicas_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
        {
            w.Write("isLastReplicaUpMessage={2}, isFromFMM={3} \r\n processed={0}, \r\ndropped={1}, \r\n",
                replicaList_, droppedReplicas_, isLastReplicaUpMessage_, isFromFMM_);
        }

        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_04(replicaList_, droppedReplicas_, isLastReplicaUpMessage_, isFromFMM_);

    private:

        // The contents of this list depend on the type of the message:
        // - For ReplicaUp message, this contains the list of Up replica.
        // - For NodeUp message, this contains the list of Up or Down replicas.
        // - For ReplicaUpReply and NodeUpAck messages, this contains the list of processed replicas.
        std::vector<Reliability::FailoverUnitInfo> replicaList_;

        // For ReplicaUp and NodeUp messages, this contains the list of replicas that are Dropped on the RA.
        // For ReplicaUpReply and NodeUpReply messages, this contains the list of replicas that are Dropped on the FM.
        std::vector<Reliability::FailoverUnitInfo> droppedReplicas_;

        // Is this the Last Replica Up message or not
        // This is sent from the RA when it sends the last message in the replica up messages
        // The FM reply to this must have this flag set as well
        bool isLastReplicaUpMessage_;

        // Indicates whether this message is from FMM
        bool isFromFMM_;
    };
}
