// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    namespace JoiningPhase
    {
        enum Enum
        {
            QueryingNeighborhood = 0,
            Locking = 1,
            EstablishingLease = 2,
            UnLocking = 3,
            Routing = 4,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
    };

    struct JoinLockTransfer;
    class JoinLockInfo;

    /////////////////////////////////////////////////////////////////////
    /// <summary>   
    ///     The join logic is implemented by the join manager.     
    /// </summary>
    /////////////////////////////////////////////////////////////////////
    class JoinManager : public Common::TextTraceComponent<Common::TraceTaskCodes::Join>
    {
        typedef std::unique_ptr<JoinLockInfo> JoinLockInfoUPtr;

        DENY_COPY(JoinManager);

    public:
        ///
        /// <summar> Constructor for the Join Manager </summary>
        ///
        /// <param name="siteNode"> The site node that owns this join manager </param>
        JoinManager(SiteNode & siteNode);

        ///
        /// <summary> Start the JoinManager. The manager can only be used once it is started.
        /// It should be called from SiteNode.Open. </summary>
        ///
        /// <param name="timeout"> The timespan after which the start operation should timeout. </param>
        /// <returns>True if start completed successfully, false otherwise.</returns>
        bool Start(Common::TimeSpan timeout);

        ///
        /// <summary> Stops the JoinManager. The manager would stop processing messaging once 
        /// Stop is called. It should be called from SiteNode.Close(). </summary>
        void Stop();

        /// <summary> The function handles a one way lock transfer request message.
        /// Using logic detailed in the comments inside the funciton, the function
        /// determines whether or not to release locks. If locks are to be released
        /// a LockTransferResponse is sent with all the request locks.</summary>
        /// 
        /// <param name=message""> The message containing the lock request.</param>
        /// <param name="oneWayReceiverContext"> The context of the received one way mesage.</param>
        void LockTransferRequestHandler(
            __in Transport::Message & message, 
            OneWayReceiverContext & oneWayReceiverContext);

        /// <summary> The function handles a one way unlock ack message.</summary>
        /// 
        /// <param name=message""> The message containing the lock request.</param>
        /// <param name="oneWayReceiverContext"> The context of the received one way mesage.</param>
        void UnLockGrantHandler(
            __in Transport::Message & message, 
            OneWayReceiverContext & oneWayReceiverContext);

        /// <summary> The function handles a one way unlock ack message.</summary>
        /// 
        /// <param name=message""> The message containing the lock request.</param>
        /// <param name="oneWayReceiverContext"> The context of the received one way mesage.</param>
        void UnLockDenyHandler(
            __in Transport::Message & message, 
            OneWayReceiverContext & oneWayReceiverContext);


        /// <summary> The function handles a one way lock grant message.
        /// It updates the corresponding partner node in the vote manager state, with the
        /// state in the message </summary>
        /// 
        /// <param name=message""> The message containing the lock grant.</param>
        /// <param name="oneWayReceiverContext"> The context of the received one way mesage.</param>
        void LockGrantHandler(
            __in Transport::Message & message,
            OneWayReceiverContext & oneWayReceiverContext);

        /// <summary> The function handles a one way lock deny message.
        /// It updates the corresponding partner node in the vote manager state, with the
        /// state in the message </summary>
        /// 
        /// <param name=message""> The message containing the lock grant.</param>
        /// <param name="oneWayReceiverContext"> The context of the received one way mesage.</param>
        void LockDenyHandler(
            __in Transport::Message & message,
            OneWayReceiverContext & oneWayReceiverContext);

        /// <summary> The function handles a one way lock response message.
        /// It updates the corresponding partner node in the vote manager state, with the
        /// state in the message </summary>
        /// 
        /// <param name=message""> The message containing the lock request.</param>
        /// <param name="oneWayReceiverContext"> The context of the received one way mesage.</param>
        void LockTransferReplyHandler(
            __in Transport::Message & message,
            OneWayReceiverContext & oneWayReceiverContext);

        void ResumeJoinHandler(
            __in Transport::Message & message,
            OneWayReceiverContext & oneWayReceiverContext);

    private:
        void RunStateMachine();

        bool CheckCompletion();

        void RestartJoining();

        void CompleteNeighborhoodQuery();

        void ResetStateForJoining();
    
        int GetPendingCount() const;

        bool IsRangeCovered();

        void StartNewPhase();

        void UpdateNeighborhood(StateMachineActionCollection & actions);

        void SendNeighborhoodQuery();
        void OnThrottleTimeout(int64 throttleSequence);

        void OnNeighborhoodQueryRequestCompleted(Common::AsyncOperationSPtr const& sendRequestOperation);
        bool IsSafeToCompleteJoinQuery();

        void StartLeaseEstablish();

        void LeaseEstablishHandler(Common::AsyncOperationSPtr const & operation, NodeInstance const & thisInstance, NodeInstance const & partner);

        void ProcessTransferReuqest(std::vector<JoinLockTransfer> const & request, NodeId requestorId, std::vector<JoinLockTransfer> & response);

        JoinLockInfo* GetLock(NodeId nodeId);

        JoinLockInfo* GetLock(NodeInstance const & nodeInstance);

        void RemoveLock(NodeId nodeId);

        void DumpState() const;

        /// <summary> The site node which owns this vote manager. </summary>
        SiteNode & site_; 

        /// <summary> The lock to be acquired before accessing the state. </summary>
        RWLOCK(Federation.JoinManager, stateLock_);

        /// <summary> The lock to be acquired before accessing the state. </summary>
        Common::TimeoutHelper timeoutHelper_;

        /// <summary> The timer to run the state machine. </summary>
        Common::TimerSPtr timer_;

        JoiningPhase::Enum phase_;

        Common::StopwatchTime insertStartTime_;

        std::vector<JoinLockInfoUPtr> entries_;

        Common::DateTime lastStateDump_;

        int waitRoutingNodeCount_;

        int64 throttleSequence_;
        NodeInstance throttleFrom_;
        bool throttled_;
        bool resumed_;
        bool queryAfterResume_;
    };

    struct JoinLockTransfer : public Serialization::FabricSerializable
    {
    public:
        JoinLockTransfer()
        {
        }

        JoinLockTransfer(JoinLock const & lock, NodeInstance const & origin, bool held)
            : lock_(lock), origin_(origin), held_(held)
        {
        }

        __declspec(property(get=getLock)) JoinLock const & Lock;
        __declspec(property(get=getOrigin)) NodeInstance const & Origin;
        __declspec(property(get=getIsHeld)) bool IsHeld;

        JoinLock const & getLock() const { return lock_; }
        NodeInstance const & getOrigin() const { return origin_; }
        bool getIsHeld() const { return held_; }

        FABRIC_FIELDS_03(lock_, origin_, held_);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const & option) const;

    private:
        JoinLock lock_;
        NodeInstance origin_;
        bool held_;
    };

    struct JoinLockTransferBody : public Serialization::FabricSerializable
    {
        JoinLockTransferBody ()
        {
        }

        JoinLockTransferBody(std::vector<JoinLockTransfer> && transfers)
            : transfers_(std::move(transfers))
        {
        }

        std::vector<JoinLockTransfer> TakeTransfers()
        {
            return std::move(this->transfers_);
        }

        FABRIC_FIELDS_01(transfers_);

    private:

        std::vector<JoinLockTransfer> transfers_;
    };

    class JoinLockInfo
    {
        DENY_COPY(JoinLockInfo);

    public:
        JoinLockInfo(PartnerNodeSPtr const & partner, bool isLocking);

        __declspec(property(get=getNodeId)) NodeId Id;
        NodeId getNodeId() const { return partner_->Id; }

        __declspec(property(get=getNodeInstance)) NodeInstance Instance;
        NodeInstance getNodeInstance() const { return partner_->Instance; }

        __declspec(property(get=getRetryCount, put=setRetryCount)) int RetryCount;
        int getRetryCount() const { return retryCount_; }
        void setRetryCount(int value) { retryCount_ = value; }

        PartnerNodeSPtr const & GetPartner() const { return partner_; }

        int64 GetLockId() const { return lock_.Id; }

        NodeIdRange GetRange() const { return lock_.HoodRange; }

        void SetKnown();

        Common::StopwatchTime GetLastSent() const { return lastSent_; }

        Common::StopwatchTime GetAcquiredTime() const { return acquiredTime_; }

        bool IsCompleted() const { return isPhaseCompleted_ && isKnown_; }

        JoinLockTransfer CreateTransfer() const { return JoinLockTransfer(lock_, partner_->Instance, isPhaseCompleted_); }

        void Complete() { isPhaseCompleted_ = true; }

        void Reset();

        bool AcquireLock(JoinLock const & lock);

        void GenerateActions(JoiningPhase::Enum phase, StateMachineActionCollection & actions);

        void Release(StateMachineActionCollection & actions);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

    private:
        PartnerNodeSPtr partner_; // The partner node which the lock originates from.
        JoinLock lock_; // The lock information.
        bool isPhaseCompleted_; // Whether the current phase has been completed for this node.
        bool isKnown_; // Whether the joining node instance is known
        Common::StopwatchTime lastSent_; // The last time this message  was sent.
        Common::StopwatchTime acquiredTime_; // The time the lock is acquired.
        int retryCount_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Federation::JoinLockTransfer);
