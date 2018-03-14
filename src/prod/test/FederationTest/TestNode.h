// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FederationTest
{
    class FederationTestSession;
    class FederationTestDispatcher;
    class TestFederation;

    class TestNode : public std::enable_shared_from_this<TestNode>
    {
        DENY_COPY(TestNode)

    public:
        TestNode(FederationTestDispatcher & dispatcher, TestFederation & federation, Federation::NodeConfig const & nodeConfig, Common::FabricCodeVersion const & codeVersion, std::wstring const & fd, Store::IStoreFactorySPtr const & storeFactory, bool checkForLeak, bool expectJoinError);

        ~TestNode();

        __declspec (property(get=get_SiteNodePtr)) std::shared_ptr<Federation::SiteNode> SiteNodePtr;

        int GetLeaseAgentPort() const;

        void Open(Common::TimeSpan, bool retryOpen, Common::EventHandler handler);
        void Close();
        void Abort();
        void Consider(Federation::FederationPartnerNodeHeader const& partnerNodeHeader, bool isInserting);

        void SendOneWayMessage(Federation::NodeId const& nodeIdTo, std::wstring const & ringName);
        void SendRequestReplyMessage(Federation::NodeId const& nodeIdTo, Common::TimeSpan, std::wstring const & ringName);
        
        void Broadcast(std::set<std::wstring> const & nodes, bool isReliable, bool toAllRings);
        void BroadcastRequest(std::set<std::wstring> const & nodes, Common::TimeSpan replyTimeout);
        
        void Multicast(
            std::vector<Federation::NodeInstance> const & targets,
            std::vector<Federation::NodeInstance> const & expectedFailedTargets, 
            std::vector<Federation::NodeInstance> const & pendingTargets, 
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout);

        void RouteOneWayMessage(Federation::NodeId const& nodeIdTo, uint64 instance, std::wstring const & ringName, Federation::NodeId const& closestSiteNode, Common::TimeSpan, Common::TimeSpan, bool expectReceived,
            bool useExactRouting, bool idempotent, Common::ErrorCodeValue::Enum expectedErrorCode);

        void RouteRequestReplyMessage(Federation::NodeId const& nodeIdTo, uint64 instance, std::wstring const & ringName, Federation::NodeId const& closestSiteNode, Common::TimeSpan, Common::TimeSpan, bool expectReceived,
            bool useExactRouting, bool idempotent, Common::ErrorCodeValue::Enum expectedErrorCode);

        void ReadVoterStore(std::wstring const & key, int64 expectedValue, int64 expectedSequence, bool expectFailure);
        void WriteVoterStore(std::wstring const & key, int64 value, int64 checkSequence, bool allowConflict, bool isStale, bool expectFailure);

        void SetMessageHandlerRejectError(Common::ErrorCode error)
        {
            messageHandlerRejectError_ = error;
        }

        std::shared_ptr<Federation::SiteNode> get_SiteNodePtr() const 
        { 
            return siteNodePtr_ ;
        }

		std::wstring GetState(std::vector<std::wstring> const & params);

        void GetGlobalTimeState(int64 & epoch, Common::TimeSpan & interval, bool & isAuthority);

    private:
        FederationTestDispatcher & dispatcher_;
        TestFederation & federation_;
        Federation::SiteNodeSPtr siteNodePtr_;
        Federation::NodeConfig nodeConfig_;
        Transport::SecuritySettings securitySettings_;
        int retryCount_;

        static std::size_t const MaxDuplicateListSize;
        static int const MaxRetryCount;

        bool isAborted_;
        bool checkForLeak_;
        bool expectJoinError_;

        Common::ErrorCode messageHandlerRejectError_;

        static Common::ExclusiveLock DuplicateCheckListLock;
        static std::list<Transport::MessageId> DuplicateCheckList;
        static bool IsDuplicate(Transport::MessageId messageId);

        Common::ExclusiveLock duplicateCheckListLock_;
        std::list<Transport::MessageId> duplicateCheckList_;
        bool IsLocalDuplicate(Transport::MessageId messageId);

        void OnOpenComplete(Common::AsyncOperationSPtr const &, bool retryOpen);

        void ReadVoterStoreCallback(Common::AsyncOperationSPtr const & operation, std::wstring const & key, int64 expectedValue, int64 expectedSequence, bool expectFailure);
        void WriteVoterStoreCallback(Common::AsyncOperationSPtr const & operation, std::wstring const & key, int64 value, int64 checkSequence, bool allowConflict, bool isStale, bool expectFailure);

        void OneWayHandler(Transport::Message & message, Federation::OneWayReceiverContext & oneWayReceiverContext);
        void RequestReplyHandler(Transport::Message & message, Federation::RequestReceiverContext & requestReceiverContext);

        void CheckForSiteNodeLeak();

        class BroadcastReplyReceiver;
    };

    typedef std::shared_ptr<TestNode> TestNodeSPtr;
};
