// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace std;
using namespace Naming;

StringLiteral const TraceTestComponent("FabricTest");

void TestUpgradeFabricData::StartUpgradeFabric(wstring const& codeVersion, wstring const& configVersion, bool isForceRestart)
{
    FabricVersion currentVersion = GetGlobalFabricVersion();
    wstring codeVersionTemp = codeVersion.empty() ? currentVersion.CodeVersion.ToString() : codeVersion;
    wstring configVersionTemp = configVersion.empty() ? currentVersion.ConfigVersion.ToString() : configVersion;

    FabricVersion newVersion;
    TestSession::FailTestIfNot(FabricVersion::TryParse(wformatString("{0}:{1}", codeVersionTemp, configVersionTemp), newVersion), "Failed to parse {0}:{1}", codeVersionTemp, configVersionTemp);
    SetGlobalFabricVersion(newVersion);
    isUpgradePending_ = true;
    isForceRestart_ = isForceRestart;
} 

void TestUpgradeFabricData::MarkUpgradeComplete()
{
    TestSession::FailTestIfNot(isUpgradePending_, "Fabric is not upgrading in MarkUpgradeComplete");
    isUpgradePending_ = false;
    waitEvent_->Set();
} 

void TestUpgradeFabricData::MarkRollbackComplete()
{
    TestSession::FailTestIfNot(isRollbackPending_, "Fabric is not rolling back in MarkRollbackComplete");
    isRollbackPending_ = false;
    waitEvent_->Set();
} 

void TestUpgradeFabricData::InterruptUpgrade()
{     
    vector<wstring> & completedDomains = CompletedUpgradeDomains;
    completedDomains.clear();

    if(isUpgradePending_ || isRollbackPending_)
    {   
        waitEvent_->Reset();
    }

    if(waitEvent_)
    {
        TestSession::FailTestIfNot(waitEvent_->WaitOne(TimeSpan::FromSeconds(60)), "Waited in InterruptCheckUpgrade for 60 seconds");
    }
} 

void TestUpgradeFabricData::RollbackUpgrade(std::wstring const& oldCodeVersion, std::wstring const& oldConfigVersion)
{     
    TestSession::FailTestIfNot(waitEvent_->IsSet(), "RollbackUpgrade called when some thread already waiting to interrupt upgrade");

    vector<wstring> & completedDomains = CompletedUpgradeDomains;
    completedDomains.clear();

    TestSession::FailTestIfNot(isUpgradePending_, "Upgrade should be pending for Fabric during rollback");

    waitEvent_->Reset();

    FabricVersion currentVersion = GetGlobalFabricVersion();
    wstring codeVersionTemp = oldCodeVersion.empty() ? currentVersion.CodeVersion.ToString() : oldCodeVersion;
    wstring configVersionTemp = oldConfigVersion.empty() ? currentVersion.ConfigVersion.ToString() : oldConfigVersion;

    FabricVersion newVersion;
    TestSession::FailTestIfNot(FabricVersion::TryParse(wformatString("{0}:{1}", codeVersionTemp, configVersionTemp), newVersion), "Failed to parse {0}:{1}", codeVersionTemp, configVersionTemp);
    SetGlobalFabricVersion(newVersion);
    isUpgradePending_ = false;
    isRollbackPending_ = true;

    if(waitEvent_)
    {
        TestSession::FailTestIfNot(waitEvent_->WaitOne(TimeSpan::FromSeconds(60)), "Waited in RollbackUpgrade for 60 seconds");
    }
} 

bool TestUpgradeFabricData::HasUpgradeBeenInterrupted()
{       
    TestSession::FailTestIfNot(isUpgradePending_ || isRollbackPending_, "Upgrade or Rollback not pending in HasUpgradeBeenInterrupted");
    if(!waitEvent_->IsSet())
    {
        // Upgrade has been interrupted
        waitEvent_->Set();
        return true;
    }

    return false;
} 

bool TestUpgradeFabricData::IsUpgradeOrRollbackPending()
{
    return isUpgradePending_ || isRollbackPending_;
} 

bool TestUpgradeFabricData::IsUpgradeOrRollbackPending(bool & isForceRestart)
{     
    if(isUpgradePending_ || isRollbackPending_)
    {
        isForceRestart = isForceRestart_;
        return true;
    }
    else
    {
        return false;
    }
} 

bool TestUpgradeFabricData::IsUpgrading()
{
    return isUpgradePending_;
}

bool TestUpgradeFabricData::IsRollingback()
{
    return isRollbackPending_;
}

void TestUpgradeFabricData::SetRollingUpgradeMode(FABRIC_ROLLING_UPGRADE_MODE mode)
{
    rollingUpgradeMode_ = mode;
}

void TestUpgradeFabricData::SetUpgradeState(FABRIC_UPGRADE_STATE state)
{
    upgradeState_ = state;
}

void TestUpgradeFabricData::AddCompletedUpgradeDomain(wstring const& upgradeDomain)
{       
    vector<wstring> & completedDomains = CompletedUpgradeDomains;
    auto findIter = find(completedDomains.begin(), completedDomains.end(), upgradeDomain);

    if (findIter == completedDomains.end())
    {
        completedDomains.push_back(upgradeDomain);
    }
}

bool TestUpgradeFabricData::AreUpgradeDomainsComplete(vector<wstring> const& upgradeDomains)
{       
    bool complete = true;
    vector<wstring> & completedDomains = CompletedUpgradeDomains;

    for (auto iter = upgradeDomains.begin(); iter != upgradeDomains.end(); ++iter)
    {
        wstring const & upgradeDomain = *iter;
        auto findIter = find(completedDomains.begin(), completedDomains.end(), upgradeDomain);
        if (findIter == completedDomains.end())
        {
            complete = false;
            break;
        }
    }

    return complete;
}

bool TestUpgradeFabricData::CheckVersion()
{
    FabricNodeConfigSPtr tempConfigSPtr;

    bool bResult = true;
    FabricVersion currentVersion = GetGlobalFabricVersion();
    FABRICSESSION.FabricDispatcher.Federation.ForEachFabricTestNode([&](FabricTestNodeSPtr const& tempNodeSPtr)
    {
        shared_ptr<FabricNodeWrapper> fabricNodeWrapper;
        if(tempNodeSPtr->TryGetFabricNodeWrapper(fabricNodeWrapper))
        {
            tempConfigSPtr = fabricNodeWrapper->Config;
            if ( tempConfigSPtr->NodeVersion != GetFabricVersionInstance(tempNodeSPtr->Node->Id) || 
                tempConfigSPtr->NodeVersion.Version != currentVersion)
            {
                bResult = false;
                return;
            }        
        }
    });    
    return bResult;
}

bool TestUpgradeFabricData::VerifyUpgrade(
    vector<wstring> const & upgradeDomains, 
    FABRIC_ROLLING_UPGRADE_MODE upgradeMode,
    vector<FABRIC_UPGRADE_STATE> const & upgradeStates)
{
    bool verified = false;

    if (upgradeDomains.empty())
    {
        verified = !this->IsUpgradeOrRollbackPending() && this->CheckVersion();
    }
    else
    {
        verified = this->AreUpgradeDomainsComplete(upgradeDomains);
    }

    if (verified && upgradeMode != FABRIC_ROLLING_UPGRADE_MODE_INVALID)
    {
        verified = (rollingUpgradeMode_ == upgradeMode);
    }

    if (verified && !upgradeStates.empty())
    {
        verified = (find(upgradeStates.begin(), upgradeStates.end(), upgradeState_) != upgradeStates.end());
    }

    return verified;
}

bool TestUpgradeFabricData::SetGlobalFabricVersion(FabricVersion inFabricVersion)
{
    gFabricVersion_ = inFabricVersion;
    return true;
}

Common::FabricVersion TestUpgradeFabricData::GetGlobalFabricVersion()
{
    return gFabricVersion_;
}


bool TestUpgradeFabricData::SetFabricVersionInstance(Federation::NodeId nodeId, Common::FabricVersionInstance fabVerInst)
{
    AcquireWriteLock grab(nodesVersionInstMapLock_);    
    nodesVersionInstMap_[nodeId] = fabVerInst;
    WriteNoise(TraceTestComponent, "set node {0} to version {1}", nodeId, fabVerInst);
    return true;
}

FabricVersionInstance TestUpgradeFabricData::GetFabricVersionInstance(Federation::NodeId nodeId)
{
    AcquireWriteLock grab(nodesVersionInstMapLock_);    
    auto iterator = nodesVersionInstMap_.find(nodeId);
    if(iterator != nodesVersionInstMap_.end())
    {
        WriteNoise(TraceTestComponent, "getversion node {0}, version {1}", nodeId, iterator->second);
        return iterator->second;
    }
    else return FabricVersionInstance(gFabricVersion_, 0);
}
