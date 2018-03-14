// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class TestUpgradeFabricData : protected Common::TextTraceComponent<Common::TraceTaskCodes::TestSession>
    {
        DENY_COPY(TestUpgradeFabricData)

    public:     
        TestUpgradeFabricData()
        : isUpgradePending_(false), 
        isRollbackPending_(false),
        isForceRestart_(false),         
        completedUpgradeDomains_(), 
        gFabricVersion_(*FabricVersion::Default),
        waitEvent_(make_shared<ManualResetEvent>(true))
        {
        }

        void StartUpgradeFabric(std::wstring const& codeVersion, std::wstring const& configVersion, bool isForceRestart=false);
        void MarkUpgradeComplete();
        void InterruptUpgrade();
        bool HasUpgradeBeenInterrupted();

        void RollbackUpgrade(std::wstring const& oldCodeVersion, std::wstring const& oldConfigVersion);
        void MarkRollbackComplete();

        bool IsUpgrading();
        bool IsRollingback();
        bool IsUpgradeOrRollbackPending();
        bool IsUpgradeOrRollbackPending(bool & isForceRestart);

        void SetRollingUpgradeMode(FABRIC_ROLLING_UPGRADE_MODE);
        void SetUpgradeState(FABRIC_UPGRADE_STATE);
        void AddCompletedUpgradeDomain(std::wstring const& upgradeDomain);
        bool AreUpgradeDomainsComplete(std::vector<std::wstring> const& upgradeDomains);
        bool CheckVersion();
        bool VerifyUpgrade(
            std::vector<std::wstring> const & upgradeDomains, 
            FABRIC_ROLLING_UPGRADE_MODE,
            std::vector<FABRIC_UPGRADE_STATE> const &);

        inline bool SetGlobalFabricVersion(Common::FabricVersion);
        inline Common::FabricVersion GetGlobalFabricVersion();
        bool SetFabricVersionInstance(Federation::NodeId nodeId, Common::FabricVersionInstance fabVerInst);
        Common::FabricVersionInstance GetFabricVersionInstance(Federation::NodeId nodeId);

    private:

        __declspec (property(get=get_IsForceRestart, put=put_IsForceRestart)) bool IsForceRestart;
        bool get_IsForceRestart() const { return isForceRestart_; }
        void put_IsForceRestart(bool value) { isForceRestart_ = value; }

        __declspec (property(get=get_WaitEvent)) std::shared_ptr<Common::ManualResetEvent> const& WaitEvent;
        std::shared_ptr<Common::ManualResetEvent> const& get_WaitEvent() const { return waitEvent_; }

        __declspec (property(get=get_CompletedUpgradeDomains)) std::vector<wstring> & CompletedUpgradeDomains;
        std::vector<wstring> & get_CompletedUpgradeDomains() { return completedUpgradeDomains_; }

        bool isUpgradePending_;
        bool isRollbackPending_;
        bool isForceRestart_;        
        FABRIC_ROLLING_UPGRADE_MODE rollingUpgradeMode_;
        FABRIC_UPGRADE_STATE upgradeState_;
        std::vector<wstring> completedUpgradeDomains_;
        std::shared_ptr<Common::ManualResetEvent> waitEvent_;
        Common::FabricVersion gFabricVersion_;

        std::map<Federation::NodeId, Common::FabricVersionInstance> nodesVersionInstMap_;
        Common::RwLock nodesVersionInstMapLock_;    
    };
};
