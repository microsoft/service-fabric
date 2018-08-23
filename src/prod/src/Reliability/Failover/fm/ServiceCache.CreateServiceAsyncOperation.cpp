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

ServiceCache::CreateServiceAsyncOperation::CreateServiceAsyncOperation(
    ServiceCache & serviceCache,
    ServiceDescription && serviceDescription,
    vector<ConsistencyUnitDescription> && consistencyUnitDescriptions,
    bool isCreateFromRebuild,
    bool isServiceUpdateNeeded,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
    : AsyncOperation(callback, parent)
    , serviceCache_(serviceCache)
    , serviceDescription_(move(serviceDescription))
    , consistencyUnitDescriptions_(move(consistencyUnitDescriptions))
    , isCreateFromRebuild_(isCreateFromRebuild)
    , isServiceUpdateNeeded_(isServiceUpdateNeeded)
    , service_()
    , lockedApplication_()
    , lockedServiceType_()
    , lockedService_()
    , failoverUnits_()
    , isNewApplication_(false)
    , isNewServiceType_(false)
    , isNewService_(false)
    , healthSequence_(FABRIC_INVALID_SEQUENCE_NUMBER)
{
}

ServiceCache::CreateServiceAsyncOperation::~CreateServiceAsyncOperation()
{
}

ErrorCode ServiceCache::CreateServiceAsyncOperation::End(AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<CreateServiceAsyncOperation>(operation);
    return thisPtr->Error;
}

void ServiceCache::CreateServiceAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    serviceCache_.fm_.ServiceEvents.CreateService_(serviceDescription_.Name, serviceDescription_);

    ErrorCode error(ErrorCodeValue::Success);

    // Do the service existence check upfront to avoid subsequent lock contention and other expensive computations.
    // We still need to perform the same check again if the service does not exist, because another racing thread
    // might have created the service.
    service_ = serviceCache_.GetService(serviceDescription_.Name);
    if (service_)
    {
        if (service_->OperationLSN == 0)
        {
            // The service only exists in memory and has not been persisted yet.
            // Wait for the previous CreateService operation to complete.
            error = ErrorCodeValue::UpdatePending;
        }
        else if (!service_->IsDeleted)
        {
            error = (service_->IsToBeDeleted ? ErrorCodeValue::FMServiceDeleteInProgress : ErrorCodeValue::FMServiceAlreadyExists);
        }
        else if (serviceDescription_.Instance <= service_->Instance)
        {
            error = ErrorCodeValue::StaleRequest;
        }
    }

    if (!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
        return;
    }


    if (!isCreateFromRebuild_)
    {
        ASSERT_IFNOT(
            static_cast<int>(consistencyUnitDescriptions_.size()) == serviceDescription_.PartitionCount,
            "PartitionCount and the number of ConsistencyUnitDescriptions do not match.");
    }

    error = serviceCache_.CreateOrGetLockedApplication(
        serviceDescription_.Type.ApplicationId,
        NamingUri(serviceDescription_.ApplicationName),
        serviceDescription_.PackageVersionInstance.InstanceId,
        0, // update id default value
        ApplicationCapacityDescription(),
        ServiceModel::ServicePackageResourceGovernanceMap(),
        ServiceModel::CodePackageContainersImagesMap(),
        lockedApplication_,
        isNewApplication_);

    if (!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
        return;
    }

    if (!isCreateFromRebuild_ &&
        !lockedApplication_->Upgrade &&
        serviceDescription_.PackageVersionInstance.InstanceId < lockedApplication_->InstanceId)
    {
        // Package instance information comes from the CM rather than Naming.
        // Distinguish the package instance (CM) from the service instance (Naming)
        // using this error code so that Naming fails this create service
        // request back to the CM rather than retrying.
        TryComplete(thisSPtr, ErrorCodeValue::StaleServicePackageInstance);
        return;
    }
    
    error = serviceCache_.CreateOrGetLockedServiceType(
        serviceDescription_.Type,
        lockedApplication_.Entry,
        lockedServiceType_,
        isNewServiceType_);
    if (!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
        return;
    }

    if (!isNewApplication_)
    {
        lockedApplication_.Release();
    }

    if (serviceDescription_.Name != Constants::TombstoneServiceName)
    {
        healthSequence_ = InterlockedIncrement64(&serviceCache_.healthSequence_) - 1;
    }

    error = serviceCache_.CreateOrGetLockedService(
        serviceDescription_,
        lockedServiceType_.Get(),
        healthSequence_,
        isServiceUpdateNeeded_,
        lockedService_,
        isNewService_);
    if (error.IsSuccess())
    {
        service_ = lockedService_.Get();

        if (!isNewService_)
        {
            if (!lockedService_->IsDeleted)
            {
                error = lockedService_->IsToBeDeleted ? ErrorCodeValue::FMServiceDeleteInProgress : ErrorCodeValue::FMServiceAlreadyExists;
            }
            else if (serviceDescription_.Instance <= lockedService_->Instance)
            {
                error = ErrorCodeValue::StaleRequest;
            }
            else
            {
                ServiceDescription serviceDescription = serviceDescription_;
                service_ = make_shared<ServiceInfo>(*lockedService_, move(serviceDescription));
                service_->ServiceType = lockedServiceType_.Get();
                service_->HealthSequence = healthSequence_;
                service_->IsServiceUpdateNeeded = false;
                service_->IsToBeDeleted = false;
                service_->IsDeleted = false;
                service_->IsForceDelete = false;
                service_->ClearFailoverUnitIds();
            }
        }
    }

    if (!error.IsSuccess())
    {
        if (healthSequence_ != FABRIC_INVALID_SEQUENCE_NUMBER)
        {
            serviceCache_.ReportServiceHealth(nullptr, error, healthSequence_);
        }

        Revert();

        TryComplete(thisSPtr, error);
        return;
    }

    int64 plbUpdateServiceTypeDuration(0);
    if (isNewServiceType_)
    {
        serviceCache_.PLBUpdateServiceType(*lockedServiceType_, plbUpdateServiceTypeDuration);
    }
    else
    {
        lockedServiceType_.Release();
    }
    
    int64 plbUpdateServiceDuration;
    error = serviceCache_.PLBUpdateService(*service_, plbUpdateServiceDuration);
    if (!error.IsSuccess())
    {
        if (healthSequence_ != FABRIC_INVALID_SEQUENCE_NUMBER)
        {
            serviceCache_.ReportServiceHealth(service_, error);
        }

        Revert();

        TryComplete(thisSPtr, error);
        return;
    }

    failoverUnits_ = CreateFailoverUnitsFromService(service_);
    for (FailoverUnitUPtr const& failoverUnit : failoverUnits_)
    {
        ASSERT_IFNOT(
            failoverUnit->PersistenceState == PersistenceState::ToBeInserted,
            "The FailoverUnit to be inserted does not have ToBeInserted state");
    }

    int64 totalPlbDuration = plbUpdateServiceDuration + plbUpdateServiceTypeDuration;
    AsyncCallback callback = [this](AsyncOperationSPtr const& updateOperation)
    {
        OnStoreUpdateCompleted(updateOperation);
    };

    
    serviceCache_.fm_.Store.BeginUpdateServiceAndFailoverUnits(
        (isNewApplication_ ? lockedApplication_.Get() : nullptr),
        (isNewServiceType_ ? lockedServiceType_.Get() : nullptr),
        *service_,
        failoverUnits_,
        OperationContextCallback<int64>(callback, move(totalPlbDuration)),
        thisSPtr);
}

void ServiceCache::CreateServiceAsyncOperation::OnStoreUpdateCompleted(AsyncOperationSPtr const& updateOperation)
{
    unique_ptr<OperationContext<int64>> context = updateOperation->PopOperationContext<int64>();

    if (context)
    {
        int64 commitDuration;
        ErrorCode error = serviceCache_.fm_.Store.EndUpdateServiceAndFailoverUnits(
            updateOperation,
            (isNewApplication_ ? lockedApplication_.Get() : nullptr),
            (isNewServiceType_ ? lockedServiceType_.Get() : nullptr),
            *service_,
            failoverUnits_,
            commitDuration);

        CreateServiceCommitJobItemUPtr commitJobItem = make_unique<CreateServiceCommitJobItem>(updateOperation->Parent, error, commitDuration, context->Context);
        serviceCache_.fm_.CommitQueue.Enqueue(move(commitJobItem));
    }
}

void ServiceCache::CreateServiceAsyncOperation::FinishCreateService(AsyncOperationSPtr const& thisSPtr, ErrorCode error, int64 commitDuration, int64 plbDuration)
{
    if (healthSequence_ != FABRIC_INVALID_SEQUENCE_NUMBER)
    {
        serviceCache_.ReportServiceHealth(service_, error);
    }

    for (FailoverUnitUPtr const& failoverUnit : failoverUnits_)
    {
        serviceCache_.fm_.FailoverUnitCacheObj.ReportHealthAfterFailoverUnitUpdate(*failoverUnit, error.IsSuccess());
    }

    if (error.IsSuccess())
    {
        if (isNewApplication_)
        {
            serviceCache_.fm_.WriteInfo(
                Constants::AppUpdateSource, lockedApplication_->IdString,
                "Application added: {0}", *lockedApplication_);
        }

        if (isNewServiceType_)
        {
            serviceCache_.fm_.WriteInfo(
                Constants::ServiceTypeSource, wformatString(lockedServiceType_->Type),
                "ServiceType added: {0}", *lockedServiceType_);
        }

        if (isNewService_)
        {
            serviceCache_.fm_.ServiceEvents.ServiceAdded(serviceDescription_.Name, *service_, commitDuration, plbDuration);
        }
        else
        {
            serviceCache_.InternalUpdateServiceInfoCache(lockedService_, move(service_), commitDuration, plbDuration);
        }

        for (size_t i = 0; i < failoverUnits_.size(); i++)
        {
            serviceCache_.fm_.FailoverUnitCacheObj.InsertFailoverUnitInCache(move(failoverUnits_[i]));
        }

        for (auto const& consistencyUnitDescription : consistencyUnitDescriptions_)
        {
            serviceCache_.fm_.ServiceEvents.ServiceCreated(serviceDescription_.Name, serviceDescription_.Instance, consistencyUnitDescription.ConsistencyUnitId.ToString());

            // Push data into Event Store.
            serviceCache_.fm_.ServiceEvents.ServiceCreatedOperational(
                Common::Guid::NewGuid(),
                serviceDescription_.Name,
                serviceDescription_.Type.ServiceTypeName,
                serviceDescription_.ApplicationName,
                serviceDescription_.ApplicationId.ApplicationTypeName,
                serviceDescription_.Instance,
                serviceDescription_.IsStateful,
                serviceDescription_.PartitionCount,
                serviceDescription_.TargetReplicaSetSize,
                serviceDescription_.MinReplicaSetSize,
                wformatString(serviceDescription_.PackageVersion),
                consistencyUnitDescription.ConsistencyUnitId.Guid);
        }
    }
    else
    {
        int64 plbDeleteDuration;
        serviceCache_.PLBDeleteService(serviceDescription_.Name, plbDeleteDuration);

        Revert();
    }

    TryComplete(thisSPtr, error);
}

vector<FailoverUnitUPtr> ServiceCache::CreateServiceAsyncOperation::CreateFailoverUnitsFromService(ServiceInfoSPtr & service)
{
    vector<FailoverUnitUPtr> failoverUnits;

    for (size_t i = 0; i < consistencyUnitDescriptions_.size(); i++)
    {
        FailoverUnitId failoverUnitId = FailoverUnitId(consistencyUnitDescriptions_[i].ConsistencyUnitId.Guid);

        service->AddFailoverUnitId(failoverUnitId);

        if (!isCreateFromRebuild_)
        {
            auto failoverUnit = new FailoverUnit(
                failoverUnitId,
                consistencyUnitDescriptions_[i],
                service->ServiceDescription.IsStateful,
                service->ServiceDescription.HasPersistedState,
                service->Name,
                service);

            failoverUnit->HealthSequence = serviceCache_.fm_.FailoverUnitCacheObj.GetHealthSequence();
            serviceCache_.fm_.FailoverUnitCacheObj.ServiceLookupTable.UpdateLookupVersion(*failoverUnit);

            failoverUnits.push_back(move(FailoverUnitUPtr(failoverUnit)));
        }
    }

    return failoverUnits;
}

void ServiceCache::CreateServiceAsyncOperation::Revert()
{
    AcquireWriteLock grab(serviceCache_.lock_);

    if (isNewService_)
    {
        lockedService_.IsDeleted = true;
        serviceCache_.services_.erase(serviceDescription_.Name);
    }

    if (isNewServiceType_)
    {
        int64 plbDeleteServiceTypeDuration;
        serviceCache_.PLBDeleteServiceType(lockedServiceType_->Type.QualifiedServiceTypeName, plbDeleteServiceTypeDuration);

        lockedServiceType_.IsDeleted = true;
        serviceCache_.serviceTypes_.erase(lockedServiceType_->Type);
    }

    if (isNewApplication_)
    {
        lockedApplication_.IsDeleted = true;
        serviceCache_.applications_.erase(lockedApplication_->ApplicationId);
    }
}

