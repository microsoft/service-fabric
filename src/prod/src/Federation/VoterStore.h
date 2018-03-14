// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class SerializedVoterStoreEntry;
    typedef std::unique_ptr<SerializedVoterStoreEntry> SerializedVoterStoreEntryUPtr;

    class VoterStoreEntry
    {
    protected:
        VoterStoreEntry() {}

    public:
        virtual ~VoterStoreEntry() {}
        virtual SerializedVoterStoreEntryUPtr Serialize() const = 0;
        virtual Common::ErrorCode Update(SerializedVoterStoreEntry const & value) = 0;
        virtual void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const = 0;
    };

    typedef std::unique_ptr<VoterStoreEntry> VoterStoreEntryUPtr;

    class SerializedVoterStoreEntry : public SerializableWithActivation
    {
    protected:
        SerializedVoterStoreEntry(SerializableWithActivationTypes::Enum typeId);

    public:
        virtual SerializedVoterStoreEntryUPtr Clone() const = 0;
        virtual VoterStoreEntryUPtr Deserialize() const = 0;
        virtual void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const = 0;
    };

    class VoterStoreSequenceEntry : public VoterStoreEntry, public SerializedVoterStoreEntry
    {
    public:
        VoterStoreSequenceEntry();
        VoterStoreSequenceEntry(int64 value);

        __declspec (property(get = getValue)) int64 Value;
        int64 getValue() const
        {
            return value_;
        }

        virtual SerializedVoterStoreEntryUPtr Serialize() const;
        virtual Common::ErrorCode Update(SerializedVoterStoreEntry const & value);
        virtual SerializedVoterStoreEntryUPtr Clone() const;
        virtual VoterStoreEntryUPtr Deserialize() const;

        virtual void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

        static VoterStoreSequenceEntry const* Convert(SerializedVoterStoreEntry const & value);

        FABRIC_FIELDS_01(value_);

    private:
        int64 value_;
    };

    class VoterStoreWStringEntry : public VoterStoreEntry, public SerializedVoterStoreEntry
    {
    public:
        VoterStoreWStringEntry(std::wstring const & value)
            : SerializedVoterStoreEntry(SerializableWithActivationTypes::WString), value_(value)
        {
        }

        __declspec (property(get = getValue)) std::wstring const & Value;
        std::wstring const & getValue() const
        {
            return value_;
        }

        virtual SerializedVoterStoreEntryUPtr Serialize() const;
        virtual Common::ErrorCode Update(SerializedVoterStoreEntry const & value);
        virtual SerializedVoterStoreEntryUPtr Clone() const;
        virtual VoterStoreEntryUPtr Deserialize() const;

        virtual void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

        static VoterStoreWStringEntry const* Convert(SerializedVoterStoreEntry const & value);

        FABRIC_FIELDS_01(value_);

    private:
        std::wstring value_;
    };

    class VoterStoreRequestAsyncOperation : public Common::TimedAsyncOperation
    {
        DENY_COPY(VoterStoreRequestAsyncOperation);

    public:
        VoterStoreRequestAsyncOperation(
            SiteNode & site,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);
        virtual void OnPrimaryResolved(NodeInstance const & primary, std::shared_ptr<VoterStoreRequestAsyncOperation> const & thisSPtr) = 0;

    protected:
        void SendRequest(Transport::MessageUPtr && request, NodeInstance const & target, std::shared_ptr<VoterStoreRequestAsyncOperation> const & thisSPtr);

        virtual void ProcessReply(Transport::MessageUPtr && reply, NodeInstance const & target, std::shared_ptr<VoterStoreRequestAsyncOperation> const & thisSPtr) = 0;

    private:
        void Resolve(std::shared_ptr<VoterStoreRequestAsyncOperation> const & thisSPtr);
        void RequestCallback(Common::AsyncOperationSPtr const & operation, NodeInstance const & target);

        SiteNode & site_;
    };

    typedef std::shared_ptr<VoterStoreRequestAsyncOperation> VoterStoreRequestAsyncOperationSPtr;

    class VoterStoreReadWriteAsyncOperation : public Common::TimedAsyncOperation
    {
        DENY_COPY(VoterStoreReadWriteAsyncOperation);

    public:
        VoterStoreReadWriteAsyncOperation(
            SiteNode & site,
            std::wstring const & key,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);
        SerializedVoterStoreEntryUPtr const & GetResult() const { return result_; }

    protected:
        virtual SerializedVoterStoreEntryUPtr GenerateValue(SerializedVoterStoreEntryUPtr const & currentValue) = 0;

    private:
        void StartRead(Common::AsyncOperationSPtr const & thisSPtr);
        void StartWrite(Common::AsyncOperationSPtr const & thisSPtr, SerializedVoterStoreEntryUPtr && value);
        void OnReadCompleted(Common::AsyncOperationSPtr const & operation);
        void OnWriteCompleted(Common::AsyncOperationSPtr const & operation);

        virtual bool ShouldRetry();

        SiteNode & site_;
        std::wstring key_;
        int64 checkSequence_;
        SerializedVoterStoreEntryUPtr result_;
    };

	class VoterStore
	{
    public:
        struct ReplicaSetConfiguration : public Serialization::FabricSerializable
        {
            DENY_COPY(ReplicaSetConfiguration);

        public:
            ReplicaSetConfiguration() {}

            ReplicaSetConfiguration & operator = (ReplicaSetConfiguration && other);

            FABRIC_FIELDS_03(Generation, Epoch, Replicas);

            int64 Generation;
            int64 Epoch;
            std::vector<NodeInstance> Replicas;
        };

        VoterStore(SiteNode & site);

        void Start();
        void Stop();

        void UpdateVoterConfig();

        Common::AsyncOperationSPtr BeginRead(
            std::wstring const & key,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
  
        Common::ErrorCode EndRead(Common::AsyncOperationSPtr const & operation, __out SerializedVoterStoreEntryUPtr & value, __out int64 & sequence);

        Common::AsyncOperationSPtr BeginWrite(
            std::wstring const & key,
            int64 checkSequence,
            SerializedVoterStoreEntryUPtr && value,
            bool isIdempotent,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndWrite(Common::AsyncOperationSPtr const & operation, __out SerializedVoterStoreEntryUPtr & value, __out int64 & sequence);

        void GetConfiguration(ReplicaSetConfiguration & config) const;

        SerializedVoterStoreEntryUPtr GetEntry(std::wstring const & key) const;

        void Resolve(VoterStoreRequestAsyncOperationSPtr const & operation);
        void ReportDownVoter(NodeInstance const & target);

        void ProcessRequestMessage(Transport::Message & request, RequestReceiverContextUPtr & requestReceiverContext);
        void ProcessOneWayMessage(Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);

        void AddHeader(Transport::Message & message);
        void SendMessage(Transport::MessageUPtr && request, NodeInstance const & target, bool isRequest);

		void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

	private:
        struct StorePhase
        {
            enum Enum
            {
                Uninitialized,
                Invalid,
                None,
                Introduce,
                Bootstrap,
                BecomeSecondary,
                Secondary,
                BecomePrimary,
                Primary
            };
        };

        struct PendingWriteRequest
        {
            DENY_COPY(PendingWriteRequest);

        public:
            PendingWriteRequest(int64 checkSequence, SerializedVoterStoreEntryUPtr && value, RequestReceiverContextUPtr && context);
            PendingWriteRequest(PendingWriteRequest && other);
            PendingWriteRequest & operator = (PendingWriteRequest && other);

            int64 CheckSequence;
            SerializedVoterStoreEntryUPtr Value;
            RequestReceiverContextUPtr Context;
        };

        struct StoreEntry
        {
            DENY_COPY(StoreEntry);

        public:
            StoreEntry();
            StoreEntry(StoreEntry && other);
            StoreEntry & operator = (StoreEntry && other);

            SerializedVoterStoreEntryUPtr Serialize() const;

            int64 Sequence;
            VoterStoreEntryUPtr Current;
            int64 PendingSequence;
            VoterStoreEntryUPtr Pending;
            RequestReceiverContextUPtr ActiveRequest;
            std::queue<PendingWriteRequest> PendingRequests;
        };

        struct Voter
        {
            Voter(NodeInstance const & instance);

            void AddSendMessageAction(StateMachineActionCollection & actions, Transport::MessageUPtr && message, VoterStore & store, bool isRequest) const;

            NodeInstance Instance;
            bool IsDown;
            mutable Common::DateTime LastSend;
            mutable int64 LastActionSequence;
        };

        struct VoterInfo : public Voter
        {
            explicit VoterInfo(NodeInstance const & instance);

            bool IsAcked;
            bool IsIntroduced;
            bool IsStale;
        };

        struct Replica : public Voter
        {
            explicit Replica(NodeInstance const & instance);

            int64 Sequence;
        };

        class ReplicaSet
        {
            DENY_COPY(ReplicaSet);

        public:
            ReplicaSet();

            __declspec (property(get = getGeneration, put = setGeneration)) int64 Generation;
            int64 getGeneration() const { return generation_; }
            void setGeneration(int64 value) { generation_ = value; }

            __declspec (property(get = getEpoch, put=setEpoch)) int64 Epoch;
            int64 getEpoch() const { return epoch_; }
            void setEpoch(int64 value) { epoch_ = value; }

            __declspec (property(get = getReplicas)) std::vector<Replica> & Replicas;
            std::vector<Replica> & getReplicas() { return replicas_; }
            std::vector<Replica> const & getReplicas() const { return replicas_; }

            Replica* GetReplica(NodeInstance const & instance);
            Replica* GetReplica(NodeId const & nodeId);
            Replica* GetLeader();
            Replica* GetPrimary();
            void GetDownReplicas(std::vector<NodeInstance> & downReplicas);
            void GetConfiguration(ReplicaSetConfiguration & config) const;
            size_t GetUpReplicaCount() const;

            bool IsActive() const;
            bool IsValid() const;
            bool HasSecondary() const;
            void ResetLastSend();

            int64 GetCommitedSequence(int64 highestSequence) const;

            bool Update(ReplicaSetConfiguration const & config);

            bool Add(NodeInstance const & target);
            void RemoveDownReplicas();
            void Clear();

        private:
            int64 generation_;
            int64 epoch_;
            std::vector<Replica> replicas_;
        };

        bool IsActiveReplica() const;

        Transport::MessageUPtr ProcessConfigQueryRequest();
        void ProcessConfigQueryReply(Transport::Message & reply);
        Transport::MessageUPtr ProcessIntroduceRequest(Transport::Message & request, NodeInstance const & target);
        void ProcessIntroduceReply(Transport::Message & reply, NodeInstance const & target);
        Transport::MessageUPtr ProcessBootstrapRequest(Transport::Message & request, NodeInstance const & target);
        void ProcessBootstrapReply(Transport::Message & reply, NodeInstance const & target);
        Transport::MessageUPtr ProcessLeaderProbeRequest(Transport::Message & request, NodeInstance const & target);
        void ProcessLeaderProbeReply(Transport::Message & reply, NodeInstance const & target);
        void ProcessJoin(Transport::Message & request, NodeInstance const & target);
        Transport::MessageUPtr ProcessProgressRequest(Transport::Message & request, NodeInstance const & target, StateMachineActionCollection & actions);
        void ProcessProgressReply(Transport::Message & reply, NodeInstance const & target);
        Transport::MessageUPtr ProcessSyncRequest(Transport::Message & request, NodeInstance const & target, StateMachineActionCollection & actions);
        void ProcessSyncReply(Transport::Message & reply, NodeInstance const & target);
        Transport::MessageUPtr ProcessReadRequest(Transport::Message & request);
        Transport::MessageUPtr ProcessWriteRequest(Transport::Message & request, RequestReceiverContextUPtr & requestReceiverContext);

        Transport::MessageUPtr ProcessWriteRequest(std::wstring const & key, int64 checkSequence, SerializedVoterStoreEntryUPtr && entry, RequestReceiverContextUPtr & requestReceiverContext);

        void ProcessMessageCommon(Transport::Message & message, NodeInstance const & target);

        Transport::MessageUPtr CreateSyncRequestMessage(int64 sequence);

        void SendCallback(Common::AsyncOperationSPtr const & operation, NodeInstance const & target, bool isRequest);
        void ProcessMessage(Transport::Message & message, NodeInstance const & target);

        void RunStateMachine();
        void CheckStablePrimary(StateMachineActionCollection & actions);
        void CheckPrimaryPromotion(StateMachineActionCollection & actions);
        void CheckReadySecondary(StateMachineActionCollection & actions);
        void CheckSecondaryPromotion(StateMachineActionCollection & actions);
        void CheckBootstrap(StateMachineActionCollection & actions);
        void CheckIntroduce(StateMachineActionCollection & actions);
        void CheckNonVoter(StateMachineActionCollection & actions);

        void InitializeVoters(std::vector<NodeId> const & seedNodes, bool hasSharedVoter);
        VoterInfo* GetVoterInfo(NodeId const & nodeId);
        VoterInfo* GetVoterInfo(NodeInstance const & instance);
        bool IsVoterDown(NodeInstance const & instance);
        void GetVoters(std::vector<NodeInstance> & voters) const;
        void UpdateVoterInstance(NodeInstance const & target, bool addNew);
        void UpdateDownVoter(NodeInstance const & target);
        void ResetVoterInfo();
        void GenerateLeaderInstance();
        void UpdateReplicaSet(ReplicaSetConfiguration const & config);
        void Bootstrap();
        void BecomePrimary();
        void CompletePendingRequests(std::vector<VoterStoreRequestAsyncOperationSPtr> const & pendingRequests, Common::ErrorCode error);

        Common::ErrorCode Write(std::wstring const & key, int64 sequence, int64 checkSequence, SerializedVoterStoreEntry const & value);

		SiteNode & site_;
		MUTABLE_RWLOCK(Federation.VoteStore, lock_);
        Common::TimerSPtr timer_;
        StorePhase::Enum phase_;
        NodeInstance leader_;
        int64 leaderInstance_;
        ReplicaSet replicaSet_;
        int64 highestSequence_;
        int64 configurationSequence_;
        Common::StopwatchTime bootstrapStartTime_;
        std::vector<VoterInfo> voters_;
        std::vector<VoterStoreRequestAsyncOperationSPtr> pendingRequests_;
        std::map<std::wstring, StoreEntry> entries_;
        std::wstring lastTracedState_;
        Common::DateTime lastTraceTime_;
    };
}
