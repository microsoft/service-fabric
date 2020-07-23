// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

ServiceCache::ProcessUpgradeAsyncOperation::ProcessUpgradeAsyncOperation(
    ServiceCache & serviceCache,
    UpgradeApplicationRequestMessageBody && requestBody,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
    : AsyncOperation(callback, parent)
    , serviceCache_(serviceCache)
    , applicationId_(requestBody.Specification.ApplicationId)
    , instanceId_(requestBody.Specification.InstanceId)
    , requestBody_(move(requestBody))
{
}

ServiceCache::ProcessUpgradeAsyncOperation::~ProcessUpgradeAsyncOperation()
{
}

ErrorCode ServiceCache::ProcessUpgradeAsyncOperation::End(
    AsyncOperationSPtr const& operation,
    __out UpgradeApplicationReplyMessageBody & replyBody)
{
    auto thisPtr = AsyncOperation::End<ProcessUpgradeAsyncOperation>(operation);
    replyBody = move(thisPtr->replyBody_);
    return thisPtr->Error;
}

void ServiceCache::ProcessUpgradeAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    LockedApplicationInfo lockedAppInfo;
    ErrorCode error = serviceCache_.GetLockedApplication(applicationId_, lockedAppInfo);
    if (!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
        return;
    }

    uint64 appInstanceId = lockedAppInfo->InstanceId;
    if (instanceId_ < appInstanceId)
    {
        TryComplete(thisSPtr, ErrorCodeValue::StaleRequest);
        return;
    }

    ApplicationUpgradeUPtr const& upgrade = lockedAppInfo->Upgrade;
    if (upgrade)
    {
        ASSERT_IF(appInstanceId != upgrade->Description.InstanceId,
            "AppInstanceId {0} does not match pending upgrade instance Id {1} for {2}",
            appInstanceId, upgrade->Description.InstanceId, lockedAppInfo->ApplicationId);

        if (instanceId_ == appInstanceId)
        {
            ApplicationUpgradeUPtr newUpgrade = make_unique<ApplicationUpgrade>(*lockedAppInfo->Upgrade);
            ApplicationUpgradeUPtr newRollback =
                (lockedAppInfo->Rollback ? make_unique<ApplicationUpgrade>(*lockedAppInfo->Rollback) : nullptr);

            bool result = newUpgrade->UpdateUpgrade(
                serviceCache_.fm_,
                move(UpgradeDescription(move(const_cast<ServiceModel::ApplicationUpgradeSpecification &>(requestBody_.Specification)))),
                requestBody_.UpgradeReplicaSetCheckTimeout,
                move(const_cast<vector<wstring> &>(requestBody_.VerifiedUpgradeDomains)),
                requestBody_.SequenceNumber);

            if (result)
            {
                ApplicationInfoSPtr newAppInfo = make_shared<ApplicationInfo>(*lockedAppInfo, move(newUpgrade), move(newRollback));

                newAppInfo->Upgrade->GetProgress(replyBody_);

                serviceCache_.BeginUpdateApplication(
                    move(lockedAppInfo),
                    move(newAppInfo),
                    true, // IsPlbUpdateNeeded
                    [this](AsyncOperationSPtr const& operation)
                    {
                        OnApplicationUpdateCompleted(operation, false);
                    },
                    thisSPtr);
            }
            else
            {
                upgrade->GetProgress(replyBody_);

                TryComplete(thisSPtr);
            }

            return;
        }
    }

    bool isNewUpgrade = (requestBody_.Specification.InstanceId > appInstanceId);

    UpgradeDomainSortPolicy::Enum sortPolicy =
        (FailoverConfig::GetConfig().SortUpgradeDomainNamesAsNumbers ? UpgradeDomainSortPolicy::DigitsAsNumbers : UpgradeDomainSortPolicy::Lexicographical);
    UpgradeDomainsCSPtr upgradeDomains = serviceCache_.fm_.NodeCacheObj.GetUpgradeDomains(sortPolicy);

    if (isNewUpgrade)
    {
        ApplicationInfoSPtr newAppInfo = nullptr;

        //if some RG settings are changing we need plb safety checks
        bool isPLBSafetyCheckNeeded = lockedAppInfo->IsPLBSafetyCheckNeeded(requestBody_.Specification.UpgradedRGSettings);

        UpgradeDescription description(move(const_cast<ServiceModel::ApplicationUpgradeSpecification &>(requestBody_.Specification)));
        ApplicationUpgradeUPtr newUpgrade = make_unique<ApplicationUpgrade>(
            move(description),
            requestBody_.UpgradeReplicaSetCheckTimeout,
            upgradeDomains,
            requestBody_.SequenceNumber,
            requestBody_.IsRollback,
            !isPLBSafetyCheckNeeded,
            FailoverConfig::GetConfig().PlbSafetyCheckTimeLimit);

        ApplicationUpgradeUPtr newRollback =
            ((requestBody_.IsRollback && lockedAppInfo->Upgrade) ? make_unique<ApplicationUpgrade>(*lockedAppInfo->Upgrade) : nullptr);

        newAppInfo = make_shared<ApplicationInfo>(*lockedAppInfo, move(newUpgrade), move(newRollback));

        newAppInfo->Upgrade->GetProgress(replyBody_);

        if (Management::NetworkInventoryManager::NetworkInventoryManagerConfig::IsNetworkInventoryManagerEnabled())
        {
            // Reference the new networks requested
            for (auto networkName : requestBody_.Specification.Networks)
            {
                ManualResetEvent completedEvent(false);

                serviceCache_.fm_.NIS.BeginNetworkAssociation(
                    networkName,
                    lockedAppInfo->ApplicationName.ToString(),
                    [this, &completedEvent, &error](AsyncOperationSPtr const & contextSPtr) mutable -> void
                {
                    error = serviceCache_.fm_.NIS.EndNetworkAssociation(contextSPtr);
                    completedEvent.Set();
                },
                    thisSPtr);

                completedEvent.WaitOne();
                if (!error.IsSuccess())
                {
                    TryComplete(thisSPtr, error.ReadValue());
                    return;
                }
            }
        }

        serviceCache_.BeginUpdateApplication(
            move(lockedAppInfo),
            move(newAppInfo),
            true, // IsPlbUpdateNeeded
            [this, isNewUpgrade](AsyncOperationSPtr const& operation)
            {
                OnApplicationUpdateCompleted(operation, isNewUpgrade);
            },
            thisSPtr);
    }
    else
    {
        vector<wstring> uds = upgradeDomains->UDs;
        replyBody_.SetCompletedUpgradeDomains(move(uds));
        serviceCache_.fm_.WriteInfo(
            FailoverManager::TraceApplicationUpgrade, lockedAppInfo->IdString,
            "Upgrade request already completed: {0}", requestBody_);

        TryComplete(thisSPtr);
    }
}

void ServiceCache::ProcessUpgradeAsyncOperation::OnApplicationUpdateCompleted(AsyncOperationSPtr const& updateOperation, bool isNewUpgrade)
{
    ErrorCode error = serviceCache_.EndUpdateApplication(updateOperation);

    if (error.IsSuccess())
    {
        if (isNewUpgrade)
        {
            serviceCache_.fm_.WriteInfo(
                FailoverManager::TraceApplicationUpgrade, wformatString(applicationId_),
                "Upgrade request accepted: Version={0}:{1}, Type={2}, IsMonitored={3}, IsManual={4}, IsRollback={5}, CheckTimeout={6}, SeqNum={7}",
                requestBody_.Specification.AppVersion,
                requestBody_.Specification.InstanceId,
                requestBody_.Specification.UpgradeType,
                requestBody_.Specification.IsMonitored,
                requestBody_.Specification.IsManual,
                requestBody_.IsRollback,
                requestBody_.UpgradeReplicaSetCheckTimeout,
                requestBody_.SequenceNumber);
        }
    }

    TryComplete(updateOperation->Parent, error);
}
