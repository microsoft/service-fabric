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

ServiceCache::ProcessPLBSafetyCheckAsyncOperation::ProcessPLBSafetyCheckAsyncOperation(
    ServiceCache & serviceCache,
    ServiceModel::ApplicationIdentifier && appId,
    Common::AsyncCallback const& callback,
    Common::AsyncOperationSPtr const& parent)
    : AsyncOperation(callback, parent)
    , serviceCache_(serviceCache)
    , appId(move(appId))
{
}

ServiceCache::ProcessPLBSafetyCheckAsyncOperation::~ProcessPLBSafetyCheckAsyncOperation()
{
}

ErrorCode ServiceCache::ProcessPLBSafetyCheckAsyncOperation::End(
    AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<ProcessPLBSafetyCheckAsyncOperation>(operation);
    return thisPtr->Error;
}

void ServiceCache::ProcessPLBSafetyCheckAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    LockedApplicationInfo lockedAppInfo;
    ErrorCode error = serviceCache_.GetLockedApplication(appId, lockedAppInfo);
    if (!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
        return;
    }

    ApplicationUpgradeUPtr const& upgrade = lockedAppInfo->Upgrade;
    //only if there is an upgrade ongoing
    if (upgrade)
    {
        //plb safety check is done
        ApplicationUpgradeUPtr newUpgrade = make_unique<ApplicationUpgrade>(*upgrade, true);
        ApplicationUpgradeUPtr newRollback =
            (lockedAppInfo->Rollback ? make_unique<ApplicationUpgrade>(*lockedAppInfo->Rollback) : nullptr);
        ApplicationInfoSPtr newAppInfo = make_shared<ApplicationInfo>(*lockedAppInfo, move(newUpgrade), move(newRollback));
        serviceCache_.BeginUpdateApplication(
            move(lockedAppInfo),
            move(newAppInfo),
            false,
            [this](AsyncOperationSPtr const& operation)
            {
                OnApplicationUpdateCompleted(operation);
            },
            thisSPtr);
    }
    //we received an update from PLB even though this application is not in upgrade or we still did not commit the upgrade
    else
    {
        error = ErrorCodeValue::ApplicationNotUpgrading;
        TryComplete(thisSPtr, error);
        return;
    }
}

void ServiceCache::ProcessPLBSafetyCheckAsyncOperation::OnApplicationUpdateCompleted(AsyncOperationSPtr const& updateOperation)
{
    ErrorCode error = serviceCache_.EndUpdateApplication(updateOperation);

    TryComplete(updateOperation->Parent, error);
}
