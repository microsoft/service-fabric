// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    namespace ArbitrationDecision
    {
        enum Enum
        {
            None = 0,
            Grant = 1,
            Reject = 2,
            Neutral = 3,
            Delay = 4
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
    }

    class ArbitrationTable : public Common::TextTraceComponent<Common::TraceTaskCodes::Arbitration>
    {
        DENY_COPY(ArbitrationTable);
        friend class ArbitrationTests;

    public:
        struct RecordState
        {
            enum Enum
            {
                None,
                Pending,
                Confirmed,
                Reported,
                Reported2,
                Rejected,
                Expired,
                Implicit
            };
        };

        ArbitrationTable(NodeId voteId);

        Transport::MessageUPtr ProcessRequest(SiteNode & site, ArbitrationRequestBody const & request, PartnerNodeSPtr const & from);
        
        Common::TimeSpan Compact(SiteNode & site);

        void Dump(Common::TextWriter& w, Common::FormatOptions const&);
        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

    private:
        class NodeHistory;

        class ArbitrationRecord
        {
            DENY_COPY(ArbitrationRecord);

        public:
            ArbitrationRecord(
                NodeHistory & node,
                LeaseWrapper::LeaseAgentInstance const & monitor,
                LeaseWrapper::LeaseAgentInstance const & subject,
                int64 monitorLeaseInstance,
                int64 subjectLeaseInstance);

            __declspec(property(get = get_Decision, put = set_Decision)) ArbitrationDecision::Enum Decision;
            ArbitrationDecision::Enum get_Decision() const { return decision_; }
            void set_Decision(ArbitrationDecision::Enum value);

            void UpdateRequest(ArbitrationRequestBody const & request);

            void UpdateState(RecordState::Enum state);

            bool IsPresent() const { return decision_ != ArbitrationDecision::None; }

            bool IsExtended() const { return monitorLeaseInstance_ > 0; }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

            NodeHistory & node_;
            int64 monitorInstance_;
            LeaseWrapper::LeaseAgentInstance subject_;
            int64 monitorLeaseInstance_;
            int64 subjectLeaseInstance_;
            Common::StopwatchTime processTime_;
            Common::StopwatchTime subjectExpireTime_;
            int flags_;
            int implicitArbitration1_;
            int implicitArbitration2_;
            RecordState::Enum state_;

            ArbitrationRecord* next_;
            ArbitrationRecord* prev_;
            ArbitrationRecord* peer_;

        private:
            ArbitrationDecision::Enum decision_;
        };

        class NodeHistory
        {
        public:
            NodeHistory(LeaseWrapper::LeaseAgentInstance const & leaseAgentInstance);

            __declspec(property(get = get_Id)) std::wstring const & Id;
            std::wstring const & get_Id() const { return id_; }

            __declspec(property(get = get_Instance)) int64 Instance;
            int64 get_Instance() const { return instance_; }

            __declspec(property(get = get_LastReceived)) Common::StopwatchTime LastReceived;
            Common::StopwatchTime get_LastReceived() const { return lastReceived_; }

            __declspec(property(get = get_From)) PartnerNodeSPtr const & From;
            PartnerNodeSPtr const & get_From() const { return from_; }

            __declspec(property(get = get_SuspicionLevel)) int SuspicionLevel;
            int get_SuspicionLevel() const { return suspicionLevel_; }

            void UpdateInstance(int64 instance);
            void ReceiveRequest(PartnerNodeSPtr const & from);

            int GetReportedCount() const { return reportedCount_; }

            bool IsSuspicious(bool withKeepAlive);
            int GetSuspicionLevel();
            int GetRawSuspicionLevel() { return suspicionLevel_; };

            void UpdateRecordState(RecordState::Enum oldState, RecordState::Enum newState);

            void Add(ArbitrationRecord* record);

            void Remove(ArbitrationRecord* record);

            ArbitrationRecord* GetRecord(
                LeaseWrapper::LeaseAgentInstance const & monitor,
                LeaseWrapper::LeaseAgentInstance const & subject,
                int64 monitorLeaseInstance,
                int64 subjectLeaseInstance,
                Common::StopwatchTime timeBound,
                bool & foundHistory);

            ArbitrationRecord* GetImplicitRecord();

            ArbitrationRecord* GetSuccessRecord();

            bool Compact(Common::StopwatchTime recordTimeBound);

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

        private:
            std::wstring id_;
            int64 instance_;
            Common::StopwatchTime lastReceived_;
            ArbitrationRecord* head_;
            ArbitrationRecord* tail_;
            int weakCount_;
            int strongCount_;
            int rejectCount_;
            int reportedCount_;
            int suspicionLevel_;

            PartnerNodeSPtr from_;

            static inline bool IsWeaklySuspicious(RecordState::Enum state)
            {
                return (state == RecordState::Pending || state == RecordState::Reported2);
            }

            static inline bool IsReported(RecordState::Enum state)
            {
                return (state == RecordState::Reported || state == RecordState::Reported2 || state == RecordState::Rejected);
            }
        };

        typedef std::unique_ptr<NodeHistory> NodeHistoryUPtr;

        class DelayedResponse
        {
        public:
            DelayedResponse() {}
            void Add(Transport::MessageUPtr && message, PartnerNodeSPtr const & target);
            void Send(SiteNode & site);

        private:
            Transport::MessageUPtr message_;
            PartnerNodeSPtr target_;
        };

        Transport::MessageUPtr InternalProcessRequest(ArbitrationRequestBody const & request, PartnerNodeSPtr const & from, DelayedResponse & delayedResponse);
        void ProcessRevertRequest(ArbitrationRequestBody const & request, NodeHistory* monitor, Common::StopwatchTime timeBound, DelayedResponse & delayedResponse);
        Transport::MessageUPtr ProcessQuery(ArbitrationRequestBody const & request);
        Transport::MessageUPtr ProcessImplicitRequest(ArbitrationRequestBody const & request, NodeHistory* monitor);

        NodeHistory* GetNodeHistory(LeaseWrapper::LeaseAgentInstance const & leaseAgentInstance, bool create);

        void MakeDecision(
            ArbitrationRequestBody const & request,
            ArbitrationRecord* monitorRecord,
            ArbitrationRecord* subjectRecord,
            bool continuous,
            DelayedResponse & delayedResponse);

        void CompactInternal(Common::StopwatchTime now);

        NodeId voteId_;
        RWLOCK(Federation.ArbitrationTable, lock_);
        Common::StopwatchTime historyStartTime_;
        Common::StopwatchTime lastCompactTime_;
        std::map<std::wstring, NodeHistoryUPtr> nodes_;
        std::vector<ArbitrationRecord*> delayed_;
    };

    void WriteToTextWriter(Common::TextWriter & w, ArbitrationTable::RecordState::Enum const & val);
} // end namespace Federation
