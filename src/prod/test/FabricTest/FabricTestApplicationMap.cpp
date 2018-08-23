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

class FabricTestApplicationMap::TesApplicationData
{
    DENY_COPY(TesApplicationData);

public:
    TesApplicationData(std::wstring const& appName, ServiceModel::ApplicationIdentifier const& appId, std::wstring const& appBuilderName)
        : appName_(appName), 
        appId_(appId), 
        appBuilderName_(appBuilderName), 
        isUpgradePending_(false), 
        isRollbackPending_(false),
        isForceRestart_(false), 
        isDeleted_(false), 
        completedUpgradeDomains_(), 
        interruptEvent_(make_shared<ManualResetEvent>(true))
    {
    }

    __declspec (property(get=get_AppName)) wstring const& AppName;
    wstring const& get_AppName() const { return appName_; }

    __declspec (property(get=get_AppId)) ServiceModel::ApplicationIdentifier const& AppId;
    ServiceModel::ApplicationIdentifier const& get_AppId() const { return appId_; }

    __declspec (property(get=get_AppBuilderName, put=put_AppBuilderName)) wstring const& AppBuilderName;
    wstring const& get_AppBuilderName() const { return appBuilderName_; }
    void put_AppBuilderName(wstring const& value) { appBuilderName_ = value; }

    __declspec (property(get=get_IsUpgradePending, put=put_IsUpgradePending)) bool IsUpgradePending;
    bool get_IsUpgradePending() const { return isUpgradePending_; }
    void put_IsUpgradePending(bool value) { isUpgradePending_ = value; }

        __declspec (property(get=get_IsRollbackPending, put=put_IsRollbackPending)) bool IsRollbackPending;
    bool get_IsRollbackPending() const { return isRollbackPending_; }
    void put_IsRollbackPending(bool value) { isRollbackPending_ = value; }

    __declspec (property(get=get_IsForceRestart, put=put_IsForceRestart)) bool IsForceRestart;
    bool get_IsForceRestart() const { return isForceRestart_; }
    void put_IsForceRestart(bool value) { isForceRestart_ = value; }

    __declspec (property(get=get_WaitEvent)) shared_ptr<ManualResetEvent> const& InterruptEvent;
    shared_ptr<ManualResetEvent> const& get_WaitEvent() const { return interruptEvent_; }

    __declspec (property(get=get_IsDeleted, put=put_IsDeleted)) bool IsDeleted;
    bool get_IsDeleted() const { return isDeleted_; }
    void put_IsDeleted(bool value) { isDeleted_ = value; }

    __declspec (property(get=get_CompletedUpgradeDomains)) vector<wstring> & CompletedUpgradeDomains;
    vector<wstring> & get_CompletedUpgradeDomains() { return completedUpgradeDomains_; }

    __declspec (property(get=get_UpgradeMode,put=set_UpgradeMode)) FABRIC_ROLLING_UPGRADE_MODE & UpgradeMode;
    FABRIC_ROLLING_UPGRADE_MODE & get_UpgradeMode() { return rollingUpgradeMode_; }
    void set_UpgradeMode(FABRIC_ROLLING_UPGRADE_MODE mode) { rollingUpgradeMode_ = mode; }

    __declspec (property(get=get_UpgradeState,put=set_UpgradeState)) FABRIC_APPLICATION_UPGRADE_STATE & UpgradeState;
    FABRIC_APPLICATION_UPGRADE_STATE & get_UpgradeState() { return upgradeState_; }
    void set_UpgradeState(FABRIC_APPLICATION_UPGRADE_STATE state) { upgradeState_ = state; }

private:
    wstring appName_;
    ServiceModel::ApplicationIdentifier appId_;
    wstring appBuilderName_;
    bool isUpgradePending_;
    bool isRollbackPending_;
    bool isForceRestart_;
    bool isDeleted_;
    FABRIC_ROLLING_UPGRADE_MODE rollingUpgradeMode_;
    FABRIC_APPLICATION_UPGRADE_STATE upgradeState_;
    vector<wstring> completedUpgradeDomains_;
    shared_ptr<ManualResetEvent> interruptEvent_;
};

// This will add a map entry <appName, TesApplicationData> for the new application.
// TesApplicationData will be created using temporary ApplicationNumber[0].
// A new UpdateApplicationId() call must follow after this method call in order to update ApplicationNumber.
void FabricTestApplicationMap::AddApplication(wstring const& appName, wstring const& appTypeName, wstring const& appBuilderName)
{
    AcquireWriteLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator != applicationDataMap_.end() && !iterator->second->IsDeleted, "App with name {0} already exists", appName);

    // Use ApplicationId 0 as ApplicationId is not known yet. This will be updated with actual value in method UpdateApplicationId
    ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
    applicationDataMap_[appName] = make_shared<TesApplicationData>(appName, appId, appBuilderName);
}

void FabricTestApplicationMap::UpdateApplicationId(wstring const& appName, Management::ClusterManager::ServiceModelApplicationId serviceModelAppId)
{
    AcquireWriteLock grab(applicationDataMapLock_);

    // Fail if this application does not exists.
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end() || iterator->second->IsDeleted, "App with name {0} does not exists or marked deleted.", appName);

    // convert ServiceModelApplicationId to ApplicationIdentifier type
    ServiceModel::ApplicationIdentifier newAppIdentifier;
    auto err = ServiceModel::ApplicationIdentifier::FromString(serviceModelAppId.get_Value(), newAppIdentifier);
    TestSession::FailTestIfNot(err.IsSuccess(), "Can not parse ApplicationIdentifier from '{0}'", serviceModelAppId.get_Value());
    applicationDataMap_[appName]= make_shared<TesApplicationData>(appName, newAppIdentifier, applicationDataMap_[appName]->AppBuilderName);
}

void FabricTestApplicationMap::DeleteApplication(std::wstring const& appName)
{
    AcquireWriteLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exist", appName);
    applicationDataMap_[appName]->IsDeleted = true;
}

void FabricTestApplicationMap::StartUpgradeApplication(wstring const& appName, wstring const& newAppBuilderName, bool isForceRestart)
{
    AcquireWriteLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exist", appName);
    TestSession::FailTestIfNot(applicationDataMap_[appName]->InterruptEvent->IsSet(), "StartUpgradeApplication called when some thread already waiting to interrupt upgrade");
    applicationDataMap_[appName]->AppBuilderName = newAppBuilderName;
    applicationDataMap_[appName]->IsUpgradePending = true;
    applicationDataMap_[appName]->IsForceRestart = isForceRestart;
}

void FabricTestApplicationMap::MarkUpgradeComplete(wstring const& appName)
{
    AcquireWriteLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exist", appName);
    TestSession::FailTestIfNot(applicationDataMap_[appName]->IsUpgradePending, "App with name {0} is not upgrading in MarkUpgradeComplete", appName);
    applicationDataMap_[appName]->IsUpgradePending = false;
    applicationDataMap_[appName]->InterruptEvent->Set();
} 

void FabricTestApplicationMap::MarkRollbackComplete(wstring const& appName)
{
    AcquireWriteLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exist", appName);
    TestSession::FailTestIfNot(applicationDataMap_[appName]->IsRollbackPending, "App with name {0} is not rolling back in MarkRollbackComplete", appName);
    applicationDataMap_[appName]->IsRollbackPending = false;
    applicationDataMap_[appName]->InterruptEvent->Set();
} 

void FabricTestApplicationMap::InterruptUpgrade(wstring const& appName)
{
    shared_ptr<ManualResetEvent> waitEvent;
    {
        AcquireWriteLock grab(applicationDataMapLock_);
        auto iterator = applicationDataMap_.find(appName);
        TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exist", appName);

        vector<wstring> & completedDomains = applicationDataMap_[appName]->CompletedUpgradeDomains;
        completedDomains.clear();

        if(applicationDataMap_[appName]->IsUpgradePending || applicationDataMap_[appName]->IsRollbackPending)
        {
            waitEvent = applicationDataMap_[appName]->InterruptEvent;
            waitEvent->Reset();
        }
    }

    if(waitEvent)
    {
        TestSession::FailTestIfNot(waitEvent->WaitOne(TimeSpan::FromSeconds(60)), "Waited in InterruptCheckUpgrade for 60 seconds");
    }

} 

void FabricTestApplicationMap::RollbackUpgrade(wstring const& appName, wstring const& oldAppBuilderName)
{
    shared_ptr<ManualResetEvent> waitEvent;
    {
        AcquireWriteLock grab(applicationDataMapLock_);
        auto iterator = applicationDataMap_.find(appName);
        TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exist", appName);
        TestSession::FailTestIfNot(applicationDataMap_[appName]->InterruptEvent->IsSet(), "RollbackUpgrade called when some thread already waiting to interrupt upgrade");

        vector<wstring> & completedDomains = applicationDataMap_[appName]->CompletedUpgradeDomains;
        completedDomains.clear();

        TestSession::FailTestIfNot(applicationDataMap_[appName]->IsUpgradePending, "Upgrade should be pending for App {0} during rollback", appName);

        waitEvent = applicationDataMap_[appName]->InterruptEvent;
        waitEvent->Reset();

        applicationDataMap_[appName]->IsUpgradePending = false;
        applicationDataMap_[appName]->IsRollbackPending = true;
        applicationDataMap_[appName]->AppBuilderName = oldAppBuilderName;
    }

    if(waitEvent)
    {
        TestSession::FailTestIfNot(waitEvent->WaitOne(TimeSpan::FromSeconds(60)), "Waited in RollbackUpgrade for 60 seconds");
    }

}

bool FabricTestApplicationMap::HasUpgradeBeenInterrupted(wstring const& appName)
{
    AcquireReadLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exists", appName);
    TestSession::FailTestIfNot(applicationDataMap_[appName]->IsUpgradePending || applicationDataMap_[appName]->IsRollbackPending, "Upgrade or Rollback not pending in HasUpgradeBeenInterrupted");
    if(!applicationDataMap_[appName]->InterruptEvent->IsSet())
    {
        // Upgrade has been interrupted
        applicationDataMap_[appName]->InterruptEvent->Set();
        return true;
    }

    return false;
} 

bool FabricTestApplicationMap::IsUpgradeOrRollbackPending(wstring const& appName)
{
    AcquireReadLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exists", appName);
    return applicationDataMap_[appName]->IsUpgradePending || applicationDataMap_[appName]->IsRollbackPending;
} 

bool FabricTestApplicationMap::IsUpgradeOrRollbackPending(ServiceModel::ApplicationIdentifier const& appId, bool & isForceRestart)
{
    wstring appName;
    TestSession::FailTestIfNot(TryGetAppName(appId, appName), "AppId {0} not found", appId);
    AcquireReadLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exists", appName);
    if(applicationDataMap_[appName]->IsUpgradePending || applicationDataMap_[appName]->IsRollbackPending)
    {
        isForceRestart = applicationDataMap_[appName]->IsForceRestart;
        return true;
    }
    else
    {
        return false;
    }
} 

bool FabricTestApplicationMap::IsUpgrading(std::wstring const& appName)
{
    AcquireReadLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exists", appName);
    return applicationDataMap_[appName]->IsUpgradePending;
}

bool FabricTestApplicationMap::IsRollingback(std::wstring const& appName)
{
    AcquireReadLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exists", appName);
    return applicationDataMap_[appName]->IsRollbackPending;
}

void FabricTestApplicationMap::SetRollingUpgradeMode(wstring const& appName, FABRIC_ROLLING_UPGRADE_MODE mode)
{
    AcquireWriteLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exist", appName);

    applicationDataMap_[appName]->UpgradeMode = mode;
}

void FabricTestApplicationMap::SetUpgradeState(wstring const& appName, FABRIC_APPLICATION_UPGRADE_STATE state)
{
    AcquireWriteLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exist", appName);

    applicationDataMap_[appName]->UpgradeState = state;
}

void FabricTestApplicationMap::AddCompletedUpgradeDomain(wstring const& appName, wstring const& upgradeDomain)
{
    AcquireWriteLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exist", appName);

    vector<wstring> & completedDomains = applicationDataMap_[appName]->CompletedUpgradeDomains;
    auto findIter = find(completedDomains.begin(), completedDomains.end(), upgradeDomain);

    if (findIter == completedDomains.end())
    {
        completedDomains.push_back(upgradeDomain);
    }
}

bool FabricTestApplicationMap::AreUpgradeDomainsComplete(wstring const& appName, vector<wstring> const& upgradeDomains)
{
    AcquireWriteLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exist", appName);

    bool complete = true;
    vector<wstring> & completedDomains = applicationDataMap_[appName]->CompletedUpgradeDomains;

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

bool FabricTestApplicationMap::VerifyUpgrade(
    wstring const& appName, 
    vector<wstring> const& upgradeDomains,
    FABRIC_ROLLING_UPGRADE_MODE upgradeMode,
    vector<FABRIC_APPLICATION_UPGRADE_STATE> const & upgradeStates)
{
    AcquireWriteLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exist", appName);

    auto & app = iterator->second;

    bool verified = false;

    if (upgradeDomains.empty())
    {
        verified = (!app->IsUpgradePending && !app->IsRollbackPending);
    }
    else
    {
        verified = true;

        for (auto const & iter : upgradeDomains)
        {
            auto findIter = find(
                app->CompletedUpgradeDomains.begin(), 
                app->CompletedUpgradeDomains.end(), 
                iter);

            if (findIter == app->CompletedUpgradeDomains.end())
            {
                verified = false;
                break;
            }
        }
    }

    if (verified && upgradeMode != FABRIC_ROLLING_UPGRADE_MODE_INVALID)
    {
        verified = (app->UpgradeMode == upgradeMode);
    }

    if (verified && !upgradeStates.empty())
    {
        verified = (find(upgradeStates.begin(), upgradeStates.end(), app->UpgradeState) != upgradeStates.end());
    }

    TestSession::WriteInfo(
        "FabricTestApplicationMap", 
        "Upgrade progress for {0} ({1}, {2}) (upgradePending={3} rollbackPending={4})  : {5}", 
        appName, 
        app->UpgradeMode,
        app->UpgradeState,
        app->IsUpgradePending,
        app->IsRollbackPending,
        app->CompletedUpgradeDomains);

    return verified;
}

ServiceModel::ApplicationIdentifier FabricTestApplicationMap::GetAppId(wstring const& appName)
{
    AcquireReadLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exists", appName);
    return applicationDataMap_[appName]->AppId;
} 

std::wstring FabricTestApplicationMap::GetCurrentApplicationBuilderName(wstring const& appName)
{
    AcquireReadLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exists", appName);
    return applicationDataMap_[appName]->AppBuilderName;
}

bool FabricTestApplicationMap::TryGetAppName(ServiceModel::ApplicationIdentifier const& appId, wstring & appName)
{
    AcquireReadLock grab(applicationDataMapLock_);
    for (auto it = applicationDataMap_.begin() ; it != applicationDataMap_.end(); it++ )
    {
        TestApplicationDataSPtr const& testApplicationDataSPtr = (*it).second;
        if(appId == testApplicationDataSPtr->AppId)
        {
            appName = testApplicationDataSPtr->AppName;
            return true;
        }
    }

    return false;
}

bool FabricTestApplicationMap::IsDeleted(std::wstring & appName)
{
    AcquireReadLock grab(applicationDataMapLock_);
    auto iterator = applicationDataMap_.find(appName);
    TestSession::FailTestIf(iterator == applicationDataMap_.end(), "App with name {0} does not exists", appName);
    return applicationDataMap_[appName]->IsDeleted;
}
