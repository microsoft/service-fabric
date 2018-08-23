// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FabricUpgradeManager
        {
            DENY_COPY(FabricUpgradeManager);

        public:

            FabricUpgradeManager(
                FailoverManager & fm,
                Common::FabricVersionInstance && currentVersionInstance,
                FabricUpgradeUPtr && upgrade);

            __declspec (property(get=get_Id)) std::wstring const& Id;
            std::wstring const& get_Id() const { return fm_.Id; }

            __declspec (property(get=get_CurrentVersionInstance)) Common::FabricVersionInstance const& CurrentVersionInstance;
            Common::FabricVersionInstance const& get_CurrentVersionInstance() const;

            __declspec (property(get = get_Upgrade)) FabricUpgradeUPtr const& Upgrade;
            FabricUpgradeUPtr const& get_Upgrade() const;

            Common::ErrorCode ProcessFabricUpgrade(UpgradeFabricRequestMessageBody && requestBody, UpgradeFabricReplyMessageBody & replyBody);

            void ProcessNodeFabricUpgradeReplyAsync(NodeFabricUpgradeReplyMessageBody && replyBody, Federation::NodeInstance const& from);

            void ProcessCancelFabricUpgradeReply(NodeFabricUpgradeReplyMessageBody && replyBody, Federation::NodeInstance const& from) const;

            void UpdateFabricUpgradeProgress(
                FabricUpgrade const& upgrade,
                std::map<Federation::NodeId, NodeUpgradeProgress> && pendingNodes,
                std::map<Federation::NodeId, std::pair<NodeUpgradeProgress, NodeInfoSPtr>> && readyNodes,
                std::map<Federation::NodeId, NodeUpgradeProgress> && waitingNodes,
                bool isCurrentDomainComplete);

            void AddFabricUpgradeContext(std::vector<BackgroundThreadContextUPtr> & contexts) const;

            bool IsFabricUpgradeNeeded(
                Common::FabricVersionInstance const& incomingVersionInstance,
                std::wstring const& upgradeDomain,
                __out Common::FabricVersionInstance & targetVersionInstance);

            Common::ErrorCode UpdateCurrentVersionInstance(Common::FabricVersionInstance const& currentVersionInstance);

            bool GetUpgradeStartTime(Common::DateTime & startTime) const;

        private:

            Common::ErrorCode PersistUpgradeUpdateCallerHoldsWriteLock(FabricUpgradeUPtr && newUpgrade);

            Common::ErrorCode UpdateCurrentVersionInstanceCallerHoldsWriteLock(Common::FabricVersionInstance const& currentVersionInstance);

            void CompleteFabricUpgradeCallerHoldsWriteLock();

            FailoverManager & fm_;

            Common::FabricVersionInstance currentVersionInstance_;
            FabricUpgradeUPtr upgrade_;

            MUTABLE_RWLOCK(FM.FabricUpgradeManager, lockObject_);
        };
    }
}
