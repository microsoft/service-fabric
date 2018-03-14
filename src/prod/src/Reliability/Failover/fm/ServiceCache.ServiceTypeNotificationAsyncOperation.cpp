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

ServiceCache::ServiceTypeNotificationAsyncOperation::ServiceTypeNotificationAsyncOperation(
    ServiceCache & serviceCache,
    vector<ServiceTypeInfo> && incomingInfos,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
    : AsyncOperation(callback, parent)
    , serviceCache_(serviceCache)
    , incomingInfos_(move(incomingInfos))
    , processedInfos_()
{
}

ServiceCache::ServiceTypeNotificationAsyncOperation::~ServiceTypeNotificationAsyncOperation()
{
}

ErrorCode ServiceCache::ServiceTypeNotificationAsyncOperation::End(
    AsyncOperationSPtr const& operation,
    __out vector<ServiceTypeInfo> & processedInfos)
{
    auto thisPtr = AsyncOperation::End<ServiceTypeNotificationAsyncOperation>(operation);

    processedInfos = move(thisPtr->processedInfos_);

    return thisPtr->Error;
}

void ServiceCache::ServiceTypeNotificationAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    vector<pair<ServiceTypeInfo, LockedServiceType>> serviceTypesToUpdate;

    {
        AcquireReadLock grab(serviceCache_.lock_);

        for (auto it = incomingInfos_.begin(); it != incomingInfos_.end(); ++it)
        {
            ServiceTypeInfo & info = *it;

            auto serviceType = serviceCache_.serviceTypes_.find(info.VersionedServiceTypeId.Id);

            if (serviceType != serviceCache_.serviceTypes_.end())
            {
                bool isCodePackageUpdateNeeded = true;

                ServicePackageVersionInstance servicePackageVersionInstance;
                if (serviceType->second->Get()->TryGetServicePackageVersionInstance(servicePackageVersionInstance))
                {
                    isCodePackageUpdateNeeded = (servicePackageVersionInstance < info.VersionedServiceTypeId.servicePackageVersionInstance);
                }

                if (isCodePackageUpdateNeeded)
                {
                    LockedServiceType lockedServiceType;
                    ErrorCode error = serviceType->second->Lock(serviceType->second, lockedServiceType);

                    if (error.IsSuccess())
                    {
                        if (lockedServiceType->TryGetServicePackageVersionInstance(servicePackageVersionInstance))
                        {
                            isCodePackageUpdateNeeded = servicePackageVersionInstance < info.VersionedServiceTypeId.servicePackageVersionInstance;
                        }

                        if (isCodePackageUpdateNeeded)
                        {
                            serviceTypesToUpdate.push_back(make_pair(move(info), move(lockedServiceType)));
                        }
                        else
                        {
                            processedInfos_.push_back(move(info));
                        }
                    }
                    else
                    {
                        serviceCache_.fm_.MessageEvents.ServiceTypeNotificationFailure(serviceType->first, error);
                    }
                }
                else
                {
                    processedInfos_.push_back(move(info));
                }
            }
        }
    }

    if (serviceTypesToUpdate.empty())
    {
        TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
    else
    {
        pendingCount_.store(serviceTypesToUpdate.size());

        for (auto it = serviceTypesToUpdate.begin(); it != serviceTypesToUpdate.end(); it++)
        {
            ServiceTypeInfo & info = it->first;
            LockedServiceType & lockedServiceType = it->second;

            ServiceTypeSPtr newServiceType = make_shared<ServiceType>(
                *lockedServiceType,
                info.VersionedServiceTypeId.servicePackageVersionInstance,
                info.CodePackageName);

            auto infoMover = MoveUPtr<ServiceTypeInfo>(move(make_unique<ServiceTypeInfo>(move(info))));
            auto lockedServiceTypeMover = MoveUPtr<LockedServiceType>(move(make_unique<LockedServiceType>(move(lockedServiceType))));

            auto newServiceTypePtr = newServiceType.get();

            serviceCache_.fmStore_.BeginUpdateData(
                *newServiceTypePtr,
                [this, infoMover, lockedServiceTypeMover, newServiceType](AsyncOperationSPtr const& operation) mutable
                {
					auto movedInfo = infoMover.TakeUPtr();
					auto movedLockedServiceType = lockedServiceTypeMover.TakeUPtr();

                    OnServiceTypeNotificationCompleted(operation, *movedInfo, *movedLockedServiceType, newServiceType);
                },
                thisSPtr);
        }
    }
}

void ServiceCache::ServiceTypeNotificationAsyncOperation::OnServiceTypeNotificationCompleted(
    AsyncOperationSPtr const& operation,
    ServiceTypeInfo & serviceTypeInfo,
    LockedServiceType & lockedServiceType,
    ServiceTypeSPtr & newServiceType)
{
    int64 commitDuration;
    ErrorCode error = serviceCache_.fm_.Store.EndUpdateData(*newServiceType, operation, commitDuration);

    if (error.IsSuccess())
    {
        serviceCache_.fm_.WriteInfo(
            Constants::ServiceTypeSource, wformatString(newServiceType->Type),
            "Service type updated: {0}\r\nCommit Duration = {1} ms", *newServiceType, commitDuration);

        lockedServiceType.Commit(move(newServiceType));

        AcquireWriteLock lock(lock_);
        processedInfos_.push_back(move(serviceTypeInfo));
    }
    else
    {
        serviceCache_.fm_.MessageEvents.ServiceTypeNotificationFailure(lockedServiceType->Type, error);
    }

    if (--pendingCount_ == 0)
    {
        TryComplete(operation->Parent, ErrorCodeValue::Success);
    }
}
