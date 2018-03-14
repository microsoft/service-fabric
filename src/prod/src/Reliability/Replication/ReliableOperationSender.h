// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ReliableOperationSender : public Common::ComponentRoot
        {
            DENY_COPY(ReliableOperationSender)

        public:
            ReliableOperationSender(
                REInternalSettingsSPtr const & config,
                ULONGLONG startSendWindowSize,
                ULONGLONG maxSendWindowSize,
                Common::Guid const & partitionId,
                std::wstring const & purpose,
                ReplicationEndpointId const & endpointUniqueId,
                FABRIC_REPLICA_ID const & replicaId,
                FABRIC_SEQUENCE_NUMBER lastAckedSequenceNumber);

            ReliableOperationSender(ReliableOperationSender && other);

            ~ReliableOperationSender();

            static ULONGLONG const DEFAULT_MAX_SWS_WHEN_0;
            static int const DEFAULT_MAX_SWS_FACTOR_WHEN_0;

            __declspec (property(get=get_LastAckedSequenceNumber)) FABRIC_SEQUENCE_NUMBER LastAckedSequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_LastAckedSequenceNumber() const;

            __declspec(property(get=get_EndpointUniqueId)) ReplicationEndpointId const & EndpointUniqueId;
            ReplicationEndpointId const & get_EndpointUniqueId() const { return endpointUniqueId_; }
           
            __declspec (property(get=get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }

            __declspec (property(get=get_Purpose)) std::wstring const & Purpose;
            std::wstring const & get_Purpose() const { return purpose_; }

            __declspec (property(get=get_IsActive)) bool IsActive;
            bool get_IsActive() const { return isActive_; }

            __declspec (property(get=get_AvgReceiveAckDuration)) Common::TimeSpan AverageReceiveAckDuration;
            Common::TimeSpan get_AvgReceiveAckDuration() const;

            __declspec (property(get=get_AvgApplyAckDuration)) Common::TimeSpan AverageApplyAckDuration;
            Common::TimeSpan get_AvgApplyAckDuration() const;

            _declspec (property(get = get_NotReceivedCount)) FABRIC_SEQUENCE_NUMBER NotReceivedCount;
            FABRIC_SEQUENCE_NUMBER get_NotReceivedCount() const;

            _declspec (property(get = get_ReceivedAndNotAppliedCount)) FABRIC_SEQUENCE_NUMBER ReceivedAndNotAppliedCount;
            FABRIC_SEQUENCE_NUMBER get_ReceivedAndNotAppliedCount() const;

            // Use a single method to get both these at the same time as there could be inconsistent values if read separately
            void GetProgress(
                __out FABRIC_SEQUENCE_NUMBER & lastAckedReceivedSequenceNumber,
                __out FABRIC_SEQUENCE_NUMBER & lastAckedQuorumSequenceNumber,
                __out Common::DateTime & lastAckProcessedTime,
                __in FABRIC_SEQUENCE_NUMBER completedLsn = Constants::InvalidLSN);

            void UpdateCompletedLsnCallerHoldsLock(FABRIC_SEQUENCE_NUMBER completedLsn);

            void ResetAverageStatistics();

            template <class T>
            void Open(
                T const & root,
                SendOperationCallback const & opCallback);

            void Close();

            void Add(
                ComOperationRawPtr const dataOp,
                FABRIC_SEQUENCE_NUMBER completedSeqNumber);

            void Add(
                ComOperationRawPtrVector const & operations,
                FABRIC_SEQUENCE_NUMBER completedSeqNumber);

            bool ProcessOnAck(
                FABRIC_SEQUENCE_NUMBER ackedReceivedSequenceNumber, 
                FABRIC_SEQUENCE_NUMBER ackedQuorumSequenceNumber);
            
            std::wstring ToString() const;
            // Never call from under lock_
            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            void FillEventData(Common::TraceEventContext & context) const;
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

            // CIT only
            void Test_GetPendingOperationsCopy(
                __out std::list<std::pair<ComOperationRawPtr, Common::DateTime>> & ops,
                __out bool & timerActive,
                __out size_t & sendWindowSize);
 
            // Ascending Sorted List of LSN's with the respective operations' receive ACK and apply ACK duration
            // OnAck() is invoked on every ACK and is hence expected to be fast
            // Every "RetryInterval", the computeaverageackduration is invoked to compute the average of all the times stored in the list.
            // It is here that operations that have both receive and apply ack are also cleaned up
            class OperationLatencyList
            {
            public:
                OperationLatencyList();

                typedef std::pair<Common::Stopwatch, Common::Stopwatch> ReceiveAndApplyTimePair;
                typedef std::pair<FABRIC_SEQUENCE_NUMBER, ReceiveAndApplyTimePair> LSNSendTimePair;

                __declspec (property(get=get_Count)) size_t Count;
                size_t get_Count() const { return items_.size(); }

                __declspec (property(get=get_LastApplyAck)) FABRIC_SEQUENCE_NUMBER LastApplyAck;
                FABRIC_SEQUENCE_NUMBER get_LastApplyAck() const { return lastApplyAck_; }

                void Add(FABRIC_SEQUENCE_NUMBER lsn);

                void OnAck(
                    FABRIC_SEQUENCE_NUMBER receiveAck,
                    FABRIC_SEQUENCE_NUMBER applyAck);

                void ComputeAverageAckDuration(
                    __out Common::TimeSpan & receiveAckDuration,
                    __out Common::TimeSpan & applyAckDuration);

                void Clear();

                std::vector<LSNSendTimePair> Test_GetItems() const
                {
                    return items_;
                }

            private:

                std::vector<LSNSendTimePair> items_;
                // Use this to quickly identify the item in the list to start on subsequent calls to OnAck()
                FABRIC_SEQUENCE_NUMBER lastApplyAck_;
            };

        private:

            void OnTimerCallback();
            void RemoveOperationsCallerHoldsLock(FABRIC_SEQUENCE_NUMBER lastReceivedLSN);
            void GetNextSendAndSetTimerCallerHoldsLock(
                __out bool & requestAck,
                __out std::vector<ComOperationCPtr> & operations);
            
            FABRIC_SEQUENCE_NUMBER NotReceivedCountCallerHoldsLock() const;
            FABRIC_SEQUENCE_NUMBER ReceivedAndNotAppliedCountCallerHoldsLock() const;
            
            REInternalSettingsSPtr const config_;
            ReplicationEndpointId const endpointUniqueId_;
            FABRIC_REPLICA_ID const replicaId_;
            ULONGLONG maxSendWindowSize_;
            Common::Guid const partitionId_;
            std::wstring const & purpose_;

            // Keep a list of operations with the time they were last sent
            typedef std::pair<ComOperationRawPtr, Common::DateTime> RawOperationPair;
            std::list<RawOperationPair> pendingOperations_;
            FABRIC_SEQUENCE_NUMBER completedSeqNumber_;

            // Callback called when an operation needs to be sent to the other side
            SendOperationCallback opCallback_;

            // Last ACKed numbers for in-order operations
            FABRIC_SEQUENCE_NUMBER lastAckedReceivedSequenceNumber_;
            FABRIC_SEQUENCE_NUMBER lastAckedApplySequenceNumber_;
            FABRIC_SEQUENCE_NUMBER highestOperationSequenceNumber_;

            // The maximum number of operation that should be sent on the wire
            // at one point in time.
            // Can't be higher than the maximum queue size specified in config.
            size_t sendWindowSize_;

            // If multiple callbacks are called one after another,
            // the send window size should be decreased, as the other end
            // may be behind, so there is no need to send it more items.
            // As soon as the other end becomes responsive (sends ACKs),
            // the send window size will be increased.
            bool noAckSinceLastCallback_;

            Common::DateTime lastAckProcessedTime_;

            // Bool set to false before opening and after close
            bool isActive_;

            // Timer for retrying sending the operations
            // that didn't receive ACKs in a long time
            Common::TimerSPtr retrySendTimer_;
            bool timerActive_;
            
            // Lock that protects the operations list 
            // and the timer
            MUTABLE_RWLOCK(REReliableOperationSender, lock_);
 
            OperationLatencyList ackDurationList_;

            DecayAverage averageReceiveAckDuration_;
            DecayAverage averageApplyAckDuration_;
            Common::Stopwatch timeSinceLastAverageUpdate_;
        }; // end ReliableOperationSender

        template <class T>
        void ReliableOperationSender::Open(
            T const & root, 
            SendOperationCallback const & opCallback)
        {
            ReplicatorEventSource::Events->OpSenderOpen(
                partitionId_, 
                endpointUniqueId_,
                replicaId_,
                purpose_,
                lastAckedReceivedSequenceNumber_,
                lastAckedApplySequenceNumber_);

            opCallback_ = opCallback;
        
            {
                Common::AcquireWriteLock lock(lock_);
                ASSERT_IF(isActive_, "{0}->{1}:{2}: Open called but active is true", endpointUniqueId_, replicaId_, purpose_);
                isActive_ = true;
                retrySendTimer_ = Common::Timer::Create(
                    "rrst",
                    [this, root] (Common::TimerSPtr const &) 
                    { 
                        this->OnTimerCallback(); 
                    },
                    true);
            }
        }
         
    } // end namespace ReplicationComponent
} // end namespace Reliability
