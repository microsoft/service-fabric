// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Communication
        {
            class FMMessageData
            {
                // Expensive to copy and so far there is no use case for copying this object
                DENY_COPY(FMMessageData);

            public:
                typedef std::pair<Reliability::FailoverUnitInfo, bool> ReplicaUpElement;

                FMMessageData() :
                    messageStage_(FMMessageStage::None),
                    sequenceNumber_(0)
                {
                }

                FMMessageData(FMMessageStage::Enum messageStage, Reliability::FailoverUnitInfo && ftInfo, bool isInDroppedList, int64 msgSequenceNumber) :
                    messageStage_(messageStage),
                    replicaUpBody_(std::make_pair(std::move(ftInfo), isInDroppedList)),
                    sequenceNumber_(msgSequenceNumber)
                {
                    AssertIfNotReplicaUpBodyType(messageStage_);
                }

                FMMessageData(FMMessageStage::Enum messageStage, ReplicaMessageBody && body, int64 msgSequenceNumber) :
                    messageStage_(messageStage),
                    replicaMessageBody_(std::move(body)),
                    sequenceNumber_(msgSequenceNumber)
                {
                    AssertIfNotReplicaMessageBodyType(messageStage_);
                }

                FMMessageData(FMMessageData && other) :
                    messageStage_(std::move(other.messageStage_)),
                    replicaUpBody_(std::move(other.replicaUpBody_)),
                    replicaMessageBody_(std::move(other.replicaMessageBody_)),
                    sequenceNumber_(std::move(other.sequenceNumber_))
                {
                }

                FMMessageData & operator=(FMMessageData && other)
                {
                    if (this != &other)
                    {
                        messageStage_ = std::move(other.messageStage_);
                        replicaUpBody_ = std::move(other.replicaUpBody_);
                        replicaMessageBody_ = std::move(other.replicaMessageBody_);
                        sequenceNumber_ = std::move(other.sequenceNumber_);
                    }

                    return *this;
                }

                __declspec(property(get = get_FMMessageStage)) FMMessageStage::Enum FMMessageStage;
                FMMessageStage::Enum get_FMMessageStage() const { return messageStage_; }

                __declspec(property(get = get_SequenceNumber)) int64 SequenceNumber;
                int64 get_SequenceNumber() const { return sequenceNumber_; }

                ReplicaUpElement TakeReplicaUp()
                {
                    AssertIfNotReplicaUpBodyType(messageStage_);
                    return std::move(replicaUpBody_);
                }

                ReplicaMessageBody TakeReplicaMessage()
                {
                    AssertIfNotReplicaMessageBodyType(messageStage_);
                    return std::move(replicaMessageBody_);
                }

            private:
                static void AssertIfNotReplicaUpBodyType(FMMessageStage::Enum e)
                {
                    ASSERT_IF(e != FMMessageStage::ReplicaUp && 
                        e != FMMessageStage::ReplicaUpload &&
                        e != FMMessageStage::ReplicaDropped && 
                        e != FMMessageStage::ReplicaDown, 
                        "expected {0} to be ReplicaUpload or ReplicaUp or ReplicaDropped or replicadown", e);
                }

                static void AssertIfNotReplicaMessageBodyType(FMMessageStage::Enum e)
                {
                    ASSERT_IF(e != FMMessageStage::EndpointAvailable, "expected {0} to be EndpointAvailable", e);
                }

                FMMessageStage::Enum messageStage_;

                ReplicaUpElement replicaUpBody_;
                ReplicaMessageBody replicaMessageBody_;
                int64 sequenceNumber_;
            };
        }
    }
}



