// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        template <typename TDescription>
        class Upgrade : public Serialization::FabricSerializable
        {
            DEFAULT_COPY_CONSTRUCTOR(Upgrade);
            DENY_COPY_ASSIGNMENT(Upgrade);

        public:

            Upgrade();

            Upgrade(
                TDescription && description,
                Common::TimeSpan upgradeReplicaSetCheckTimeout,
                UpgradeDomainsCSPtr const& upgradeDomains,
                int64 sequenceNumber,
                bool isRollback,
                bool isPLBSafetyCheckDone = true,
                Common::TimeSpan plbSafetyCheckTimeout = Common::TimeSpan::Zero);

            Upgrade(Upgrade && other);

            Upgrade & operator = (Upgrade && other);

            __declspec (property(get = get_Description)) TDescription const& Description;
            TDescription const& get_Description() const { return description_; }

            __declspec (property(get = get_ClusterStartedTime)) Common::DateTime ClusterStartedTime;
            Common::DateTime get_ClusterStartedTime() const { return clusterStartedTime_; }

            __declspec (property(get = get_CurrentDomainStartedTime)) Common::DateTime CurrentDomainStartedTime;
            Common::DateTime get_CurrentDomainStartedTime() const { return currentDomainStartedTime_; }

            __declspec(property(get = get_UpgradeDomains)) UpgradeDomainsCSPtr const& UpgradeDomains;
            UpgradeDomainsCSPtr const& get_UpgradeDomains() const { return upgradeDomains_; }

            __declspec(property(get = get_UpgradeDomainSortPolicy)) UpgradeDomainSortPolicy::Enum UpgradeDomainSortPolicy;
            UpgradeDomainSortPolicy::Enum get_UpgradeDomainSortPolicy() const { return sortPolicy_; }

            __declspec (property(get = get_TraceSource)) Common::StringLiteral const& TraceSource;
            virtual Common::StringLiteral const& get_TraceSource() const = 0;

            __declspec (property(get = get_TraceType)) std::wstring const& TraceType;
            virtual std::wstring get_TraceType() const = 0;

            bool IsReplicaSetCheckNeeded() const;

            bool IsCompleted() const;

            bool IsReplicaSetWaitInEffect() const;

            void GetProgress(UpgradeReplyMessageBody & replyBody) const;

            bool CheckUpgradeDomain(std::wstring const & upgradeDomain) const;

            std::set<std::wstring> GetCompletedDomains() const;

            void UpdateDomains(UpgradeDomainsCSPtr const& upgradeDomains);

            bool UpdateProgress(
                FailoverManager & fm,
                std::map<Federation::NodeId, std::pair<NodeUpgradeProgress, NodeInfoSPtr>> const& readyNodes,
                bool isUpgradeNeededOnCurrentUD,
                bool isComplete);

            bool UpdateUpgrade(
                FailoverManager & fm,
                TDescription && description,
                Common::TimeSpan upgradeReplicaSetCheckTimeout,
                std::vector<std::wstring> && verifiedDomains,
                int64 sequenceNumber);

            void UpdateProgressDetails(
                std::map<Federation::NodeId, NodeUpgradeProgress> && pendingNodes,
                std::map<Federation::NodeId, std::pair<NodeUpgradeProgress, NodeInfoSPtr>> && readyNodes,
                std::map<Federation::NodeId, NodeUpgradeProgress> && waitingNodes,
                bool isComplete);

            bool UpdateVerifiedDomains(FailoverManager & fm, std::vector<std::wstring> const & verifiedDomains);

            void AddCompletedNode(Federation::NodeId nodeId);

            bool IsDomainStarted(std::wstring const& upgradeDomain) const;

            bool IsUpgradeCompletedOnNode(NodeInfo const& nodeInfo) const;

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

            FABRIC_FIELDS_12(
                description_,
                upgradeReplicaSetCheckTimeout_,
                currentDomain_,
                clusterStartedTime_,
                currentDomainStartedTime_,
                sequenceNumber_,
                isUpgradePerformedOnCurrentUD_,
                skippedUDs_,
                sortPolicy_,
                isPLBSafetyCheckDone_,
                plbSafetyCheckTimeout_,
                isRollback_);

        protected:

            bool IsCurrentDomainStarted() const;

            virtual Transport::MessageUPtr CreateUpgradeRequestMessage() const = 0;

            TDescription description_;
            std::wstring currentDomain_;
            Common::DateTime clusterStartedTime_;
            Common::DateTime currentDomainStartedTime_;
            Common::TimeSpan upgradeReplicaSetCheckTimeout_;

            int64 sequenceNumber_;

            bool isRollback_;

            UpgradeDomainsCSPtr upgradeDomains_;
            size_t currentDomainIndex_;

            std::set<Federation::NodeId> completedNodes_;

            Common::DateTime lastUpgradeMessageSendTime_;

            std::vector<NodeUpgradeProgress> nodeUpgradeProgressList_;

            bool isUpgradePerformedOnCurrentUD_;
            std::vector<std::wstring> skippedUDs_;

            UpgradeDomainSortPolicy::Enum sortPolicy_;

            bool isPLBSafetyCheckDone_;
            Common::TimeSpan plbSafetyCheckTimeout_;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class FabricUpgrade : public Upgrade<ServiceModel::FabricUpgradeSpecification>
        {
            DEFAULT_COPY_CONSTRUCTOR(FabricUpgrade);
            DENY_COPY_ASSIGNMENT(FabricUpgrade);

        public:

            FabricUpgrade();

            FabricUpgrade(
                ServiceModel::FabricUpgradeSpecification && specification,
                Common::TimeSpan upgradeReplicaSetCheckTimeout,
                UpgradeDomainsCSPtr const& upgradeDomains,
                int64 sequenceNumber,
                bool isRollback);

            FabricUpgrade(FabricUpgrade && other);

            __declspec (property(get = get_PersistenceState, put = set_PersistenceState)) PersistenceState::Enum PersistenceState;
            PersistenceState::Enum get_PersistenceState() const { return persistenceState_; }
            void set_PersistenceState(PersistenceState::Enum value) { persistenceState_ = value; }

            virtual Common::StringLiteral const& get_TraceSource() const;
            virtual std::wstring get_TraceType() const;

            std::unique_ptr<FabricUpgradeContext> CreateContext(std::wstring const& fmId) const;

        private:

            virtual Transport::MessageUPtr CreateUpgradeRequestMessage() const;

            PersistenceState::Enum persistenceState_;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class ApplicationUpgrade : public Upgrade<UpgradeDescription>
        {
            DEFAULT_COPY_CONSTRUCTOR(ApplicationUpgrade);
            DENY_COPY_ASSIGNMENT(ApplicationUpgrade);

        public:

            ApplicationUpgrade();

            ApplicationUpgrade(
                UpgradeDescription && description,
                Common::TimeSpan upgradeReplicaSetCheckTimeout,
                UpgradeDomainsCSPtr const& upgradeDomains,
                int64 sequenceNumber,
                bool isRollback,
                bool isPLBSafetyCheckDone = true,
                Common::TimeSpan plbSafetyCheckTimeout = Common::TimeSpan::Zero);

            ApplicationUpgrade(ApplicationUpgrade const & other, bool isPLBSafetyCheckDone);

            ApplicationUpgrade(ApplicationUpgrade && other);

            virtual Common::StringLiteral const& get_TraceSource() const;
            virtual std::wstring get_TraceType() const;

            __declspec (property(get = get_IsPLBSafetyCheckDone)) bool IsPLBSafetyCheckDone;
            bool get_IsPLBSafetyCheckDone() const;

            std::unique_ptr<ApplicationUpgradeContext> CreateContext(ApplicationInfoSPtr const & application) const;

        private:
            virtual Transport::MessageUPtr CreateUpgradeRequestMessage() const;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        template <class TDescription>
        struct TrySetPendingTraits
        {
            static Common::AsyncOperationSPtr BeginTrySetPending(
                FailoverManager const& fm,
                NodeInfo const& nodeInfo,
                TDescription const& description,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            static bool EndTrySetPending(Common::AsyncOperationSPtr const& operation, FailoverManager const& fm);
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        template <>
        struct TrySetPendingTraits<ServiceModel::FabricUpgradeSpecification>
        {
            static Common::AsyncOperationSPtr BeginTrySetPending(
                FailoverManager const& fm,
                NodeInfo const& nodeInfo,
                ServiceModel::FabricUpgradeSpecification const& description,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            static bool EndTrySetPending(Common::AsyncOperationSPtr const& operation, FailoverManager const& fm);
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        template <>
        struct TrySetPendingTraits<UpgradeDescription>
        {
            static Common::AsyncOperationSPtr BeginTrySetPending(
                FailoverManager const& fm,
                NodeInfo const& nodeInfo,
                UpgradeDescription const& description,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            static bool EndTrySetPending(Common::AsyncOperationSPtr const& operation, FailoverManager const& fm);
        };
    }
}
