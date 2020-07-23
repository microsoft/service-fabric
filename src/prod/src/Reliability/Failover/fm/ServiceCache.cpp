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
using namespace Federation;

ServiceCache::ServiceCache(
    FailoverManager& fm,
    FailoverManagerStore& fmStore,
    InstrumentedPLB & plb,
    vector<ApplicationInfoSPtr> const & applications,
    vector<ServiceTypeSPtr> const & serviceTypes,
    vector<ServiceInfoSPtr> const & services)
    : fm_(fm),
    fmStore_(fmStore),
    plb_(plb),
    ackedSequence_(FABRIC_INVALID_SEQUENCE_NUMBER),
    healthInitialized_(false),
    healthSequence_(0),
    initialSequence_(0),
    invalidateSequence_(0),
    closed_(false),
    nextServiceTraceIndex_(0)
{
    for (ApplicationInfoSPtr const & application : applications)
    {
        ApplicationInfoSPtr applicationInfo = application;
        ApplicationEntrySPtr applicationEntry = make_shared<CacheEntry<ApplicationInfo>>(move(applicationInfo));
        applications_.insert(make_pair(application->ApplicationId, move(applicationEntry)));
    }

    for (ServiceTypeSPtr const & serviceType : serviceTypes)
    {
        auto it = applications_.find(serviceType->Type.ApplicationId);

        ASSERT_IF(it == applications_.end(), "Application not found for service type: {0}", serviceType->Type);

        ServiceTypeSPtr newServiceType = make_shared<ServiceType>(*serviceType, it->second);
        ServiceTypeEntrySPtr serviceTypeEntry = make_shared<CacheEntry<ServiceType>>(move(newServiceType));
        serviceTypes_.insert(make_pair(serviceType->Type, move(serviceTypeEntry)));
    }

    for (ServiceInfoSPtr const & service : services)
    {
        if (service->HealthSequence >= healthSequence_)
        {
            healthSequence_ = service->HealthSequence + 1;
        }

        ServiceTypeSPtr serviceType = GetServiceTypeCallerHoldsLock(service->ServiceDescription.Type);

        ASSERT_IF(!serviceType, "Service type not found for service: {0}", service->ServiceDescription);

        ServiceInfoSPtr newService = make_shared<ServiceInfo>(*service);
        newService->ServiceType = serviceType;
        ServiceEntrySPtr serviceEntry = make_shared<CacheEntry<ServiceInfo>>(move(newService));
        services_.insert(make_pair(service->Name, move(serviceEntry)));
    }

    if (!fm.IsMaster)
    {
        initialSequence_ = healthSequence_;

        fm.HealthClient->HealthPreInitialize(*ServiceModel::Constants::HealthReportFMSource, FABRIC_HEALTH_REPORT_KIND_SERVICE, fm_.PreOpenFMServiceEpoch.DataLossVersion);

        fm.WriteInfo(Constants::ServiceSource, "Loaded health sequence: {0}", healthSequence_);
    }
}

void ServiceCache::Close()
{
    AcquireWriteLock grab(lock_);
    closed_ = true;
}

void ServiceCache::StartPeriodicTask()
{
    if (fm_.BackgroundManagerObj.IsAdminTraceEnabled)
    {
        TraceServices();
    }

    if (fm_.IsMaster)
    {
        return;
    }

    fm_.Events.PeriodicTaskBeginNoise(fm_.Id, PeriodicTaskName::ServiceCache);
    auto startTime = Stopwatch::Now();

    ServicePeriodicTaskCounts taskCounts;

    vector<ServiceEntrySPtr> servicesToUpdate;

    {
        AcquireWriteLock grab(lock_);

        FABRIC_SEQUENCE_NUMBER progress;
        ErrorCode error = fm_.HealthClient->HealthGetProgress(*ServiceModel::Constants::HealthReportFMSource, FABRIC_HEALTH_REPORT_KIND_SERVICE, progress);
        if (!error.IsSuccess())
        {
            return;
        }

        ackedSequence_ = progress;

        if (!healthInitialized_)
        {
            InitializeHealthState();
        }
        else if (invalidateSequence_ != FABRIC_INVALID_SEQUENCE_NUMBER)
        {
            for (auto it = services_.begin(); it != services_.end(); ++it)
            {
                ServiceInfoSPtr const& serviceInfo = it->second->Get();
                if (serviceInfo->HealthSequence < invalidateSequence_ && serviceInfo->Name != Constants::TombstoneServiceName)
                {
                    servicesToUpdate.push_back(it->second);
                }
            }

            if (servicesToUpdate.size() == 0)
            {
                invalidateSequence_ = FABRIC_INVALID_SEQUENCE_NUMBER;
            }
        }
    }

    TimeoutHelper timeout(TimeSpan::FromSeconds(1));
    for (auto it = servicesToUpdate.begin(); it != servicesToUpdate.end() && !timeout.IsExpired; ++it)
    {
        LockedServiceInfo lockedServiceInfo;
        ErrorCode error = (*it)->Lock(*it, lockedServiceInfo, TimeSpan::Zero);
        if (error.IsSuccess() && lockedServiceInfo->HealthSequence < invalidateSequence_)
        {
            ServiceInfoSPtr newServiceInfo = make_shared<ServiceInfo>(*lockedServiceInfo);
            newServiceInfo->HealthSequence = GetNextHealthSequence();
            newServiceInfo->PersistenceState = PersistenceState::ToBeUpdated;

            auto lockedServiceInfoMover = MoveUPtr<LockedServiceInfo>(move(make_unique<LockedServiceInfo>(move(lockedServiceInfo))));
            auto newServiceInfoPtr = newServiceInfo.get();

            fmStore_.BeginUpdateData(
                *newServiceInfoPtr,
                [this, lockedServiceInfoMover, newServiceInfo](AsyncOperationSPtr const& updateOperation) mutable
                {
                    int64 commitDuration(0);
                    ErrorCode error = fmStore_.EndUpdateData(*newServiceInfo, updateOperation, commitDuration);

                    ReportServiceHealth(newServiceInfo, error);

                    if (error.IsSuccess())
                    {
                        fm_.ServiceEvents.UpdatedHealthSequence(newServiceInfo->Name, *newServiceInfo, commitDuration);

                        auto lockedServiceInfo = lockedServiceInfoMover.TakeUPtr();
                        lockedServiceInfo->Commit(move(newServiceInfo));
                    }
                    else
                    {
                        fm_.ServiceEvents.UpdateHealthSequenceFailed(newServiceInfo->Name, *newServiceInfo, error, commitDuration);
                    }
                },
                fm_.CreateAsyncOperationRoot());

            taskCounts.ServicesUpdateHealthSequenceStarted++;
        }
    }

    auto duration = Stopwatch::Now() - startTime;
    fm_.Events.ServicePeriodicTaskEnd(fm_.Id, duration, taskCounts);
}

void ServiceCache::InitializeHealthState()
{
    FABRIC_SEQUENCE_NUMBER progress = ackedSequence_;

    fm_.WriteInfo(Constants::ServiceSource, "HealthGetProgress: {0}", progress);

    if (progress > initialSequence_)
    {
        fm_.WriteWarning(Constants::ServiceSource, "Progress retrieved {0} is larger than FM knows {1}",
            progress, initialSequence_);

        invalidateSequence_ = initialSequence_ = progress;

        FABRIC_SEQUENCE_NUMBER currentSequence = healthSequence_;
        while (currentSequence < progress)
        {
            FABRIC_SEQUENCE_NUMBER last = InterlockedCompareExchange64(&healthSequence_, progress, currentSequence);
            if (last == currentSequence)
            {
                fm_.WriteWarning(Constants::ServiceSource, "Updating sequence from {0} to {1}",
                    currentSequence, progress);
                break;
            }
            else
            {
                currentSequence = last;
            }
        }
    }

    size_t reportCount = 0;
    if (initialSequence_ > progress)
    {
        vector<HealthReport> reports;

        for (auto it = services_.begin(); it != services_.end(); ++it)
        {
            ServiceInfoSPtr const& serviceInfo = it->second->Get();
            if (serviceInfo->HealthSequence >= progress && serviceInfo->HealthSequence < initialSequence_ && serviceInfo->Name != Constants::TombstoneServiceName)
            {
                reports.push_back(fm_.HealthReportFactoryObj.GenerateServiceInfoHealthReport(serviceInfo));
            }
        }

        reportCount = reports.size();
        if (reportCount > 0)
        {
            fm_.HealthClient->AddHealthReports(move(reports));
        }
    }

    FABRIC_SEQUENCE_NUMBER invalidateSequence = (invalidateSequence_ == 0 ? FABRIC_INVALID_SEQUENCE_NUMBER : invalidateSequence_);
    fm_.HealthClient->HealthPostInitialize(*ServiceModel::Constants::HealthReportFMSource, FABRIC_HEALTH_REPORT_KIND_SERVICE, initialSequence_, invalidateSequence);
    fm_.WriteInfo(Constants::ServiceSource, "HealthPostInitialize {0}/{1} with {2} reports", initialSequence_, invalidateSequence, reportCount);

    healthInitialized_ = true;
}

void ServiceCache::ReportServiceHealth(ServiceInfoSPtr const & serviceInfo, ErrorCode error, FABRIC_SEQUENCE_NUMBER healthSequence)
{
    ASSERT_IF(serviceInfo && serviceInfo->Name == Constants::TombstoneServiceName,
        "reporting tomb stone service: {0}", *serviceInfo);

    if (fm_.IsMaster)
    {
        return;
    }

    if (error.IsSuccess())
    {
        fm_.HealthClient->AddHealthReport(fm_.HealthReportFactoryObj.GenerateServiceInfoHealthReport(serviceInfo));
    }
    else
    {
        if (serviceInfo)
        {
            healthSequence = serviceInfo->HealthSequence;
        }

        fm_.HealthClient->HealthSkipSequence(*ServiceModel::Constants::HealthReportFMSource, FABRIC_HEALTH_REPORT_KIND_SERVICE, healthSequence);
    }
}

void ServiceCache::GetServices(vector<ServiceInfoSPtr> & services) const
{
    AcquireReadLock grab(lock_);

    for (auto it = services_.begin(); it != services_.end(); ++it)
    {
        ServiceInfoSPtr serviceInfo = it->second->Get();
        services.push_back(serviceInfo);
    }
}

void ServiceCache::GetServiceQueryResults(ServiceCacheFilter const & filter, __out vector<ServiceQueryResult> & serviceResults) const
{
    auto currentFabricCodeVersion = fm_.FabricUpgradeManager.CurrentVersionInstance.Version.CodeVersion;

    {
        AcquireReadLock grab(lock_);

        for (auto it = services_.begin(); it != services_.end(); ++it)
        {
            ServiceInfoSPtr serviceInfo = it->second->Get();
            if (!serviceInfo->IsDeleted && filter(serviceInfo))
            {
                serviceResults.push_back(serviceInfo->ToServiceQueryResult(currentFabricCodeVersion));
            }
        }
    }
}

void ServiceCache::GetServiceGroupMemberQueryResults(ServiceCacheFilter const & filter, __out vector<ServiceGroupMemberQueryResult> & serviceGroupMemberResults) const
{
    auto currentFabricCodeVersion = fm_.FabricUpgradeManager.CurrentVersionInstance.Version.CodeVersion;

    {
        AcquireReadLock grab(lock_);

        for (auto it = services_.begin(); it != services_.end(); ++it)
        {
            ServiceInfoSPtr serviceInfo = it->second->Get();
            if (!serviceInfo->IsDeleted && filter(serviceInfo))
            {
                serviceGroupMemberResults.push_back(serviceInfo->ToServiceGroupMemberQueryResult(currentFabricCodeVersion));
            }
        }
    }
}

ErrorCode ServiceCache::GetServiceDescription(wstring const& serviceName, __out ServiceDescription & serviceDescription) const
{
    AcquireReadLock grab(lock_);

    auto it = services_.find(serviceName);
    if (it == services_.end())
    {
        return ErrorCodeValue::ServiceNotFound;
    }

    serviceDescription = it->second->Get()->ServiceDescription;
    return ErrorCode::Success();
}

ErrorCode ServiceCache::GetFMServiceForGFUMTransfer(__out ServiceInfoSPtr & serviceInfo) const
{
    ASSERT_IFNOT(fm_.IsMaster, "Only FMM request the FM service for GFUM transfer");

    auto it = services_.find(SystemServiceApplicationNameHelper::InternalFMServiceName);

    ASSERT_IF(it == services_.end(), "FM service should exist at the time of GFUM transfer");

    LockedServiceInfo lockedServiceInfo;
    ErrorCode error = it->second->Lock(it->second, lockedServiceInfo);
    if (!error.IsSuccess())
    {
        return error;
    }

    serviceInfo = lockedServiceInfo.Get();

    return ErrorCode::Success();
}

ServiceInfoSPtr ServiceCache::GetService(wstring const& serviceName) const
{
    AcquireReadLock grab(lock_);
    return GetServiceCallerHoldsLock(serviceName);
}

ServiceInfoSPtr ServiceCache::GetServiceCallerHoldsLock(wstring const& serviceName) const
{
    auto it = services_.find(serviceName);
    if (it == services_.end())
    {
        return nullptr;
    }

    ServiceEntrySPtr serviceEntry = it->second;
    ServiceInfoSPtr serviceInfo = serviceEntry->Get();
    if (!serviceInfo->ServiceType->IsStale)
    {
        return serviceInfo;
    }

    LockedServiceInfo lockedService;
    ErrorCode error = serviceEntry->Lock(serviceEntry, lockedService, TimeSpan::Zero);
    if (error.IsSuccess())
    {
        auto serviceTypeEntry = serviceTypes_.find(serviceInfo->ServiceDescription.Type);

        ASSERT_IF(
            serviceTypeEntry == serviceTypes_.end(),
            "service type {0} not found for {1}", serviceInfo->ServiceDescription.Type, serviceName);

        ServiceInfoSPtr newServiceInfo = make_shared<ServiceInfo>(*lockedService);
        newServiceInfo->ServiceType = serviceTypeEntry->second->Get();
        lockedService.Commit(move(newServiceInfo));
    }

    return serviceEntry->Get();
}

void ServiceCache::GetServiceTypes(vector<ServiceTypeSPtr> & serviceTypes) const
{
    AcquireReadLock grab(lock_);

    for (auto it = serviceTypes_.begin(); it != serviceTypes_.end(); ++it)
    {
        ServiceTypeSPtr serviceType = it->second->Get();
        serviceTypes.push_back(serviceType);
    }
}

bool ServiceCache::ServiceExists(wstring const& serviceName) const
{
    AcquireReadLock grab(lock_);
    return services_.find(serviceName) != services_.end();
}

ServiceTypeSPtr ServiceCache::GetServiceType(ServiceTypeIdentifier const& serviceTypeId) const
{
    AcquireReadLock grab(lock_);
    return GetServiceTypeCallerHoldsLock(serviceTypeId);
}

ServiceTypeSPtr ServiceCache::GetServiceTypeCallerHoldsLock(ServiceTypeIdentifier const& serviceTypeId) const
{
    auto it = serviceTypes_.find(serviceTypeId);
    if (it == serviceTypes_.end())
    {
        return nullptr;
    }

    return it->second->Get();
}

void ServiceCache::GetApplications(vector<ApplicationInfoSPtr> & applications) const
{
    AcquireReadLock grab(lock_);

    for (auto it = applications_.begin(); it != applications_.end(); ++it)
    {
        ApplicationInfoSPtr application = it->second->Get();
        applications.push_back(application);
    }
}

ApplicationInfoSPtr ServiceCache::GetApplication(ApplicationIdentifier const& applicationId) const
{
    AcquireReadLock grab(lock_);
    return GetApplicationCallerHoldsLock(applicationId);
}

ApplicationInfoSPtr ServiceCache::GetApplicationCallerHoldsLock(ApplicationIdentifier const& applicationId) const
{
    auto it = applications_.find(applicationId);
    if (it == applications_.end())
    {
        return nullptr;
    }

    return it->second->Get();
}

void ServiceCache::CleanupServiceTypesAndApplications()
{
    fm_.Events.PeriodicTaskBeginNoise(fm_.Id, PeriodicTaskName::ServiceCleanup);

    vector<LockedServiceInfo> servicesToDelete;
    vector<LockedServiceType> serviceTypesToDelete;
    vector<LockedApplicationInfo> applicationsToDelete;

    ServiceCounts counts;

    {
        AcquireReadLock grab(lock_);

        DateTime now = DateTime::Now();

        // Find ServiceInfos that can be deleted
        for (auto service = services_.begin(); service != services_.end(); ++service)
        {
            ServiceInfoSPtr serviceInfo = service->second->Get();
            if (serviceInfo->IsDeleted &&
                serviceInfo->HealthSequence + 1 < ackedSequence_ &&
                (now - serviceInfo->LastUpdated) > FailoverConfig::GetConfig().PeriodicStateCleanupScanInterval)
            {
                LockedServiceInfo lockedService;
                ErrorCode error = service->second->Lock(service->second, lockedService, TimeSpan::Zero);
                if (error.IsSuccess() && lockedService->OperationLSN == serviceInfo->OperationLSN)
                {
                    servicesToDelete.push_back(move(lockedService));
                }
            }

            // Tracing
            if (serviceInfo->ServiceDescription.IsStateful)
            {
                if (serviceInfo->ServiceDescription.IsVolatile)
                {
                    counts.VolatileCount++;
                }

                if (serviceInfo->ServiceDescription.HasPersistedState)
                {
                    counts.PersistedCount++;
                }
            }
            else
            {
                counts.StatelessCount++;
            }

            if (serviceInfo->IsServiceUpdateNeeded)
            {
                counts.UpdatingCount++;
            }

            if (serviceInfo->IsToBeDeleted)
            {
                counts.DeletingCount++;
            }

            if (serviceInfo->IsDeleted)
            {
                counts.DeletedCount++;
            }
        }

        // Find ServiceTypes that can be deleted
        for (auto serviceType = serviceTypes_.begin(); serviceType != serviceTypes_.end(); ++serviceType)
        {
            bool exists = false;
            for (auto serviceInfo = services_.begin(); serviceInfo != services_.end(); ++serviceInfo)
            {
                if (serviceInfo->second->Get()->ServiceDescription.Type == serviceType->first)
                {
                    exists = true;
                    break;
                }
            }

            if (!exists)
            {
                LockedServiceType lockedServiceType;
                ErrorCode error = serviceType->second->Lock(serviceType->second, lockedServiceType, TimeSpan::Zero);
                if (error.IsSuccess() && lockedServiceType->OperationLSN == serviceType->second->Get()->OperationLSN)
                {
                    // Check again if the ServiceType still needs to be deleted
                    exists = false;
                    for (auto serviceInfo = services_.begin(); serviceInfo != services_.end(); ++serviceInfo)
                    {
                        if (serviceInfo->second->Get()->ServiceDescription.Type == serviceType->first)
                        {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists)
                    {
                        serviceTypesToDelete.push_back(move(lockedServiceType));
                    }
                }
            }
        }

        // Find ApplicationInfos that can be deleted
        for (auto application = applications_.begin(); application != applications_.end(); ++application)
        {
            ApplicationInfoSPtr applicationInfo = application->second->Get();

            if (applicationInfo->IsDeleted)
            {
                bool exists = false;
                for (auto serviceType = serviceTypes_.begin(); serviceType != serviceTypes_.end(); ++serviceType)
                {
                    if (serviceType->second->Get()->Type.ApplicationId == application->first)
                    {
                        exists = true;
                        break;
                    }
                }

                if (!exists)
                {
                    LockedApplicationInfo lockedApplication;
                    ErrorCode error = application->second->Lock(application->second, lockedApplication, TimeSpan::Zero);
                    if (error.IsSuccess() && lockedApplication->OperationLSN == applicationInfo->OperationLSN)
                    {
                        // Check again if the application still needs to be deleted
                        exists = false;
                        for (auto serviceType = serviceTypes_.begin(); serviceType != serviceTypes_.end(); ++serviceType)
                        {
                            if (serviceType->second->Get()->Type.ApplicationId == application->first)
                            {
                                exists = true;
                                break;
                            }
                        }

                        if (lockedApplication->IsDeleted && !exists)
                        {
                            applicationsToDelete.push_back(move(lockedApplication));
                        }
                    }
                }
            }

            // Tracing
            ApplicationUpgradeUPtr const & upgrade = application->second->Get()->Upgrade;

            if (upgrade)
            {
                counts.ApplicationUpgradeCount++;

                if (upgrade->Description.IsMonitored)
                {
                    counts.MonitoredUpgradeCount++;
                }

                if (upgrade->Description.IsManual)
                {
                    counts.ManualUpgradeCount++;
                }

                if (upgrade->Description.UpgradeType == UpgradeType::Rolling_ForceRestart)
                {
                    counts.ForceRestartUpgradeCount++;
                }

                if (upgrade->Description.UpgradeType == UpgradeType::Rolling_NotificationOnly)
                {
                    counts.NotificationOnlyUpgradeCount++;
                }
            }
        }
    }

    Trace(counts);

    // Delete Services
    for (auto it = servicesToDelete.begin(); it != servicesToDelete.end(); ++it)
    {
        LockedServiceInfo & lockedService = *it;
        lockedService->PersistenceState = PersistenceState::ToBeDeleted;

        auto mover = MoveUPtr<LockedServiceInfo>(move(make_unique<LockedServiceInfo>(move(lockedService))));
        ServiceInfo & serviceInfo = *mover->Get();

        fmStore_.BeginUpdateData(
            serviceInfo,
            [this, mover](AsyncOperationSPtr const& updateOperation) mutable
            {
                int64 commitDuration(0);
                ErrorCode error = fmStore_.EndUpdateData(*mover->Get(), updateOperation, commitDuration);
                if (error.IsSuccess())
                {
                    RemoveServiceCommitJobItemUPtr commitJobItem = make_unique<RemoveServiceCommitJobItem>(move(*mover.TakeUPtr()), commitDuration);
                    fm_.CommitQueue.Enqueue(move(commitJobItem));
                }
            },
            fm_.CreateAsyncOperationRoot());
    }

    // Delete ServiceTypes
    for (auto it = serviceTypesToDelete.begin(); it != serviceTypesToDelete.end(); ++it)
    {
        LockedServiceType & lockedServiceType = *it;
        lockedServiceType->PersistenceState = PersistenceState::ToBeDeleted;

        auto mover = MoveUPtr<LockedServiceType>(move(make_unique<LockedServiceType>(move(lockedServiceType))));
        ServiceType & serviceType = *mover->Get();

        fmStore_.BeginUpdateData(
            serviceType,
            [this, mover](AsyncOperationSPtr const& updateOperation) mutable
            {
                int64 commitDuration(0);
                ErrorCode error = fmStore_.EndUpdateData(*mover->Get(), updateOperation, commitDuration);
                if (error.IsSuccess())
                {
                    RemoveServiceTypeCommitJobItemUPtr commitJobItem = make_unique<RemoveServiceTypeCommitJobItem>(move(*mover.TakeUPtr()), commitDuration);
                    fm_.CommitQueue.Enqueue(move(commitJobItem));
                }
            },
            fm_.CreateAsyncOperationRoot());
    }

    // Delete ApplicationInfos
    for (auto it = applicationsToDelete.begin(); it != applicationsToDelete.end(); ++it)
    {
        LockedApplicationInfo & lockedApplication = *it;
        lockedApplication->PersistenceState = PersistenceState::ToBeDeleted;

        auto mover = MoveUPtr<LockedApplicationInfo>(move(make_unique<LockedApplicationInfo>(move(lockedApplication))));
        ApplicationInfo & applicationInfo = *mover->Get();

        fmStore_.BeginUpdateData(
            applicationInfo,
            [this, mover](AsyncOperationSPtr const& updateOperation) mutable
            {
                int64 commitDuration(0);
                ErrorCode error = fmStore_.EndUpdateData(*mover->Get(), updateOperation, commitDuration);
                if (error.IsSuccess())
                {
                    RemoveApplicationCommitJobItemUPtr commitJobItem = make_unique<RemoveApplicationCommitJobItem>(move(*mover.TakeUPtr()), commitDuration, 0);
                    fm_.CommitQueue.Enqueue(move(commitJobItem));
                }
            },
            fm_.CreateAsyncOperationRoot());
    }
}

void ServiceCache::Trace(ServiceCounts counts) const
{
    AcquireReadLock grab(lock_);

    counts.ServiceCount = services_.size();
    counts.ServiceTypeCount = serviceTypes_.size();
    counts.ApplicationCount = applications_.size();

    fm_.ServiceEvents.ServiceCounts(counts);

    // Performance Counters
    fm_.FailoverUnitCounters->NumberOfServices.Value = static_cast<PerformanceCounterValue>(counts.ServiceCount);
    fm_.FailoverUnitCounters->NumberOfServiceTypes.Value = static_cast<PerformanceCounterValue>(counts.ServiceTypeCount);
    fm_.FailoverUnitCounters->NumberOfApplications.Value = static_cast<PerformanceCounterValue>(counts.ApplicationCount);
    fm_.FailoverUnitCounters->NumberOfApplicationUpgrades.Value = static_cast<PerformanceCounterValue>(counts.ApplicationUpgradeCount);
}

void ServiceCache::FinishRemoveService(LockedServiceInfo & lockedServiceInfo, int64 commitDuration)
{
    AcquireWriteLock grab(lock_);

    lockedServiceInfo.IsDeleted = true;
    services_.erase(lockedServiceInfo->Name);
    fm_.ServiceEvents.ServiceRemoved(lockedServiceInfo->Name, *lockedServiceInfo, commitDuration);
}

void ServiceCache::FinishRemoveServiceType(LockedServiceType & lockedServiceType, int64 commitDuration)
{
    int64 plbDuration;
    PLBDeleteServiceType(lockedServiceType->Type.QualifiedServiceTypeName, plbDuration);

    AcquireWriteLock grab(lock_);

    lockedServiceType.IsDeleted = true;
    serviceTypes_.erase(lockedServiceType->Type);
    fm_.WriteInfo(
        Constants::ServiceTypeSource, wformatString(lockedServiceType->Type),
        "Service type removed: {0}\r\nCommit Duration = {1} ms, PLB Duration = {2} ms", *lockedServiceType, commitDuration, plbDuration);
}

void ServiceCache::FinishRemoveApplication(LockedApplicationInfo & lockedApplicationInfo, int64 commitDuration, int64 plbDuration)
{
    AcquireWriteLock grab(lock_);

    lockedApplicationInfo.IsDeleted = true;
    applications_.erase(lockedApplicationInfo->ApplicationId);
    fm_.Events.AppRemoved(lockedApplicationInfo->IdString, lockedApplicationInfo->ApplicationId, commitDuration, plbDuration);
}

void ServiceCache::CompleteServiceUpdateAsync(wstring const& serviceName, uint64 serviceInstance, uint64 updateVersion)
{
    LockedServiceInfo lockedServiceInfo;
    ErrorCode error = GetLockedService(serviceName, lockedServiceInfo);
    if (!error.IsSuccess())
    {
        fm_.ServiceEvents.ServiceUpdateFailure(serviceName, error, 0, 0);
        return;
    }

    if (lockedServiceInfo->Instance != serviceInstance ||
        lockedServiceInfo->ServiceDescription.UpdateVersion != updateVersion)
    {
        // This is a stale request.
        return;
    }

    ServiceInfoSPtr newServiceInfo = make_shared<ServiceInfo>(*lockedServiceInfo);
    newServiceInfo->IsServiceUpdateNeeded = false;
    newServiceInfo->RepartitionInfo = nullptr;

    auto mover = MoveUPtr<LockedServiceInfo>(move(make_unique<LockedServiceInfo>(move(lockedServiceInfo))));

    fmStore_.BeginUpdateData(
        *newServiceInfo,
        [this, mover, newServiceInfo](AsyncOperationSPtr const& updateOperation) mutable
        {
            int64 commitDuration(0);
            ErrorCode error = fmStore_.EndUpdateData(*newServiceInfo, updateOperation, commitDuration);
            if (error.IsSuccess())
            {
                auto lockedServiceInfo = mover.TakeUPtr();
                InternalUpdateServiceInfoCache(*lockedServiceInfo, move(newServiceInfo), commitDuration, 0);
            }
            else
            {
                fm_.ServiceEvents.ServiceUpdateFailure(newServiceInfo->Name, error, commitDuration, 0);
            }
        },
        fm_.CreateAsyncOperationRoot());
}

AsyncOperationSPtr ServiceCache::BeginDeleteService(
    wstring const& serviceName,
    bool const isForce,
    uint64 serviceInstance,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<DeleteServiceAsyncOperation>(
        *this,
        serviceName,
        isForce,
        serviceInstance,
        callback,
        parent);
}

ErrorCode ServiceCache::EndDeleteService(AsyncOperationSPtr const& operation)
{
    return DeleteServiceAsyncOperation::End(operation);
}

void ServiceCache::MarkServiceAsToBeDeletedAsync(wstring const& serviceName, uint64 serviceInstance)
{
    BeginDeleteService(
        serviceName,
        false, // Unforcefully
        serviceInstance,
        [this, serviceName](AsyncOperationSPtr const& operation)
        {
            ErrorCode error = EndDeleteService(operation);
            if (!error.IsSuccess())
            {
                fm_.ServiceEvents.ServiceUpdateFailure(serviceName, error, 0, 0);
            }
        },
        fm_.CreateAsyncOperationRoot());
}

void ServiceCache::MarkServiceAsDeletedAsync(wstring const& serviceName, uint64 serviceInstance)
{
    LockedServiceInfo lockedServiceInfo;
    ErrorCode error = GetLockedService(serviceName, lockedServiceInfo);
    if (!error.IsSuccess())
    {
        fm_.ServiceEvents.ServiceUpdateFailure(serviceName, error, 0, 0);
        return;
    }

    if (lockedServiceInfo->Instance != serviceInstance)
    {
        // A new service instance has been created.
        // This is a stale request.
        return;
    }
    if (lockedServiceInfo->IsDeleted)
    {
        return;
    }

    ServiceInfoSPtr newServiceInfo = make_shared<ServiceInfo>(*lockedServiceInfo);
    newServiceInfo->IsToBeDeleted = false;
    newServiceInfo->IsDeleted = true;
    newServiceInfo->HealthSequence = GetNextHealthSequence();

    auto mover = MoveUPtr<LockedServiceInfo>(move(make_unique<LockedServiceInfo>(move(lockedServiceInfo))));

    fmStore_.BeginUpdateData(
        *newServiceInfo,
        [this, mover, newServiceInfo](AsyncOperationSPtr const& updateOperation) mutable
        {
            int64 commitDuration(0);
            ErrorCode error = fmStore_.EndUpdateData(*newServiceInfo, updateOperation, commitDuration);
            ReportServiceHealth(newServiceInfo, error);
            if (error.IsSuccess())
            {
                newServiceInfo->CompleteDeleteOperation(error);

                auto lockedServiceInfo = mover.TakeUPtr();
                int64 plbDuration;
                PLBDeleteService(newServiceInfo->ServiceDescription.Name, plbDuration);
                InternalUpdateServiceInfoCache(*lockedServiceInfo, move(newServiceInfo), commitDuration, plbDuration);
            }
            else
            {
                fm_.ServiceEvents.ServiceUpdateFailure(newServiceInfo->Name, error, commitDuration, 0);
            }
        },
        fm_.CreateAsyncOperationRoot());
}

AsyncOperationSPtr ServiceCache::BeginDeleteApplication(
    ApplicationIdentifier const& applicationId,
    uint64 instanceId,
    AsyncCallback const& callback,
    AsyncOperationSPtr const & state)
{
    LockedApplicationInfo lockedAppInfo;
    ErrorCode error = GetLockedApplication(applicationId, lockedAppInfo);
    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    if (lockedAppInfo->IsDeleted)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, state);
    }
    if (instanceId < lockedAppInfo->InstanceId)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::StaleRequest, callback, state);
    }

    ApplicationInfoSPtr newAppInfo = make_shared<ApplicationInfo>(*lockedAppInfo);
    newAppInfo->IsDeleted = true;

    int64 plbDuration;
    PLBDeleteApplication(newAppInfo->ApplicationName, plbDuration);

    if (Management::NetworkInventoryManager::NetworkInventoryManagerConfig::IsNetworkInventoryManagerEnabled())
    {
        // Disassociate the network from the app in case of failure
        auto appInfo = lockedAppInfo.Entry->Get();
        for( auto networkName : appInfo->Networks)
        {        
            ManualResetEvent completedEvent(false);

            fm_.NIS.BeginNetworkDissociation(
                networkName,
                newAppInfo->ApplicationName.ToString(),
                [&completedEvent](AsyncOperationSPtr const&) { completedEvent.Set(); },
                state);

            completedEvent.WaitOne();
        }
    }

    auto newApplicationInfoPtr = newAppInfo.get();

    return fmStore_.BeginUpdateData(
        *newApplicationInfoPtr,
        OperationContextCallback<tuple<ApplicationInfoSPtr, LockedApplicationInfo, int64>>(callback, move(make_tuple(move(newAppInfo), move(lockedAppInfo), move(plbDuration)))),
        state);
}

ErrorCode ServiceCache::EndDeleteApplication(AsyncOperationSPtr const& operation)
{
    unique_ptr<OperationContext<tuple<ApplicationInfoSPtr, LockedApplicationInfo, int64>>> context = operation->PopOperationContext<tuple<ApplicationInfoSPtr, LockedApplicationInfo, int64>>();

    if (context)
    {
        ApplicationInfoSPtr & newApplicationInfo = get<0>(context->Context);
        LockedApplicationInfo & lockedApplication = get<1>(context->Context);
        int64 & plbDuration = get<2>(context->Context);

        int64 commitDuration(0);
        ErrorCode error = fm_.Store.EndUpdateData(*newApplicationInfo, operation, commitDuration);

        PostUpdateApplication(lockedApplication, move(newApplicationInfo), commitDuration, plbDuration, error, true);

        return error;
    }

    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr ServiceCache::BeginInternalUpdateServiceInfo(
    LockedServiceInfo && lockedServiceInfo,
    ServiceInfoSPtr && newServiceInfo,
    bool isPlbUpdateNeeded,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    auto newServiceInfoPtr = newServiceInfo.get();
    int64 plbDuration(0);
    if (isPlbUpdateNeeded)
    {
        auto error = PLBUpdateService(*newServiceInfoPtr, plbDuration);
        if (!error.IsSuccess())
        {
            fm_.ServiceEvents.ServiceUpdateFailure(newServiceInfoPtr->Name, error, 0, plbDuration);
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
        }
    }

    tuple<LockedServiceInfo, ServiceInfoSPtr, bool, int64> context(move(lockedServiceInfo), move(newServiceInfo), move(isPlbUpdateNeeded), move(plbDuration));

    return fmStore_.BeginUpdateData(
        *newServiceInfoPtr,
        OperationContextCallback<tuple<LockedServiceInfo, ServiceInfoSPtr, bool, int64>>(callback, move(context)),
        state);
}

ErrorCode ServiceCache::EndInternalUpdateServiceInfo(AsyncOperationSPtr const& operation)
{
    unique_ptr<OperationContext<tuple<LockedServiceInfo, ServiceInfoSPtr, bool, int64>>> context = operation->PopOperationContext<tuple<LockedServiceInfo, ServiceInfoSPtr, bool, int64>>();

    if (context)
    {
        LockedServiceInfo & lockedServiceInfo = get<0>(context->Context);
        ServiceInfoSPtr & newServiceInfo = get<1>(context->Context);
        bool isPlbUpdateNeeded = get<2>(context->Context);
        int64 & plbDuration = get<3>(context->Context);

        int64 commitDuration(0);
        ErrorCode error = fmStore_.EndUpdateData(*newServiceInfo, operation, commitDuration);

        if (error.IsSuccess())
        {
            InternalUpdateServiceInfoCache(lockedServiceInfo, move(newServiceInfo), commitDuration, plbDuration);
        }
        else if (isPlbUpdateNeeded)
        {
            int64 secondPlbCallDuration;
            // Revert the update of PLB (forced - PLB will assert if not successful)
            PLBUpdateService(*lockedServiceInfo, secondPlbCallDuration, true);
            // Add the duration to the previous call to save the total plb call duration
            plbDuration += secondPlbCallDuration;
        }

        if (!error.IsSuccess())
        {
            fm_.ServiceEvents.ServiceUpdateFailure(newServiceInfo->Name, error, commitDuration, plbDuration);
        }

        return error;
    }

    return CompletedAsyncOperation::End(operation);
}

void ServiceCache::InternalUpdateServiceInfoCache(LockedServiceInfo & lockedServiceInfo, ServiceInfoSPtr && newServiceInfo, int64 commitDuration, int64 plbDuration)
{
    wstring const& name = newServiceInfo->Name;

    fm_.ServiceEvents.ServiceUpdate(name, *newServiceInfo, commitDuration, plbDuration);

    if (newServiceInfo->IsToBeDeleted)
    {
        fm_.ServiceEvents.ServiceToBeDeleted(name);
    }
    else if (newServiceInfo->IsDeleted)
    {
        fm_.ServiceEvents.ServiceDeleted(name, newServiceInfo->Instance);

        // Push data into Event Store.
        fm_.ServiceEvents.ServiceDeletedOperational(
            Common::Guid::NewGuid(),
            newServiceInfo->Name,
            newServiceInfo->ServiceDescription.Type.ServiceTypeName,
            newServiceInfo->ServiceDescription.ApplicationName,
            newServiceInfo->ServiceDescription.ApplicationId.ApplicationTypeName,
            newServiceInfo->ServiceDescription.Instance,
            newServiceInfo->ServiceDescription.IsStateful,
            newServiceInfo->ServiceDescription.PartitionCount,
            newServiceInfo->ServiceDescription.TargetReplicaSetSize,
            newServiceInfo->ServiceDescription.MinReplicaSetSize,
            wformatString(newServiceInfo->ServiceDescription.PackageVersion));
    }

    lockedServiceInfo.Commit(move(newServiceInfo));
}

ErrorCode ServiceCache::RemoveFromDisabledNodes(uint64 sequenceNumber, NodeId const& nodeId)
{
    ManualResetEvent completedEvent(false);

    AsyncOperationSPtr operation = BeginRemoveFromDisabledNodes(
        sequenceNumber,
        move(nodeId),
        [&completedEvent](AsyncOperationSPtr const&) { completedEvent.Set(); },
        fm_.CreateAsyncOperationRoot());

    completedEvent.WaitOne();

    return EndRemoveFromDisabledNodes(operation);
}

AsyncOperationSPtr ServiceCache::BeginRemoveFromDisabledNodes(
    uint64 sequenceNumber,
    NodeId const& nodeId,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    return AsyncOperation::CreateAndStart<RemoveFromDisabledNodesAsyncOperation>(
        *this,
        sequenceNumber,
        nodeId,
        callback,
        state);
}

ErrorCode ServiceCache::EndRemoveFromDisabledNodes(AsyncOperationSPtr const& operation)
{
    return RemoveFromDisabledNodesAsyncOperation::End(operation);
}

AsyncOperationSPtr ServiceCache::BeginProcessServiceTypeNotification(
    vector<ServiceTypeInfo> && incomingInfos,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    return AsyncOperation::CreateAndStart<ServiceTypeNotificationAsyncOperation>(
        *this,
        move(incomingInfos),
        callback,
        state);
}

ErrorCode ServiceCache::EndProcessServiceTypeNotification(
    AsyncOperationSPtr const& operation,
    __out vector<ServiceTypeInfo> & processedInfos)
{
    return ServiceTypeNotificationAsyncOperation::End(operation, processedInfos);
}

AsyncOperationSPtr ServiceCache::BeginUpdateServiceType(
    ServiceTypeEntrySPtr const& serviceTypeEntry,
    uint64 sequenceNumber,
    NodeId const& nodeId,
    ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    LockedServiceType lockedServiceType;
    ErrorCode error = serviceTypeEntry->Lock(serviceTypeEntry, lockedServiceType);
    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::UpdatePending, callback, state);
    }

    if (lockedServiceType.IsDeleted)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::StaleRequest, callback, state);
    }

    NodeInfoSPtr const& nodeInfo = fm_.NodeCacheObj.GetNode(nodeId);
    if (nodeInfo && sequenceNumber < nodeInfo->SequenceNumber)
    {
        TESTASSERT_IF(sequenceNumber < nodeInfo->SequenceNumber);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::StaleRequest, callback, state);
    }

    bool isServiceTypeDisabled = lockedServiceType->IsServiceTypeDisabled(nodeId);

    if ((serviceTypeUpdateEvent == ServiceTypeUpdateKind::Enabled && isServiceTypeDisabled) ||
        (serviceTypeUpdateEvent == ServiceTypeUpdateKind::Disabled && !isServiceTypeDisabled))
    {
        ServiceTypeSPtr newServiceType = make_shared<ServiceType>(*lockedServiceType, nodeId, serviceTypeUpdateEvent);
        auto newServiceTypePtr = newServiceType.get();

        pair<LockedServiceType, ServiceTypeSPtr> context(move(lockedServiceType), move(newServiceType));

        return fmStore_.BeginUpdateData(
            *newServiceTypePtr,
            OperationContextCallback<pair<LockedServiceType, ServiceTypeSPtr>>(callback, move(context)),
            state);
    }

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
}

ErrorCode ServiceCache::EndUpdateServiceType(AsyncOperationSPtr const& operation)
{
    unique_ptr<OperationContext<pair<LockedServiceType, ServiceTypeSPtr>>> context = operation->PopOperationContext<pair<LockedServiceType, ServiceTypeSPtr>>();

    if (context)
    {
        LockedServiceType & lockedServiceType = context->Context.first;
        ServiceTypeSPtr & newServiceType = context->Context.second;

        int64 commitDuration(0);
        ErrorCode error = fm_.Store.EndUpdateData(*newServiceType, operation, commitDuration);

        if (error.IsSuccess())
        {
            int64 plbDuration;
            PLBUpdateServiceType(*newServiceType, plbDuration);

            fm_.WriteInfo(
                Constants::ServiceTypeSource, wformatString(lockedServiceType->Type),
                "ServiceType updated: {0}\r\nCommit Duration = {1} ms, PLB Duration = {2} ms", *newServiceType, commitDuration, plbDuration);

            lockedServiceType.Commit(move(newServiceType));
        }

        return error;
    }

    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr ServiceCache::BeginUpdateServiceTypes(
    vector<ServiceTypeIdentifier> && serviceTypes,
    uint64 sequenceNumber,
    NodeId const& nodeId,
    ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    return AsyncOperation::CreateAndStart<UpdateServiceTypesAsyncOperation>(
        *this,
        move(serviceTypes),
        sequenceNumber,
        nodeId,
        serviceTypeUpdateEvent,
        callback,
        state);
}

ErrorCode ServiceCache::EndUpdateServiceTypes(AsyncOperationSPtr const& operation)
{
    return UpdateServiceTypesAsyncOperation::End(operation);
}

void ServiceCache::PostUpdateApplication(
    LockedApplicationInfo & lockedApplication,
    ApplicationInfoSPtr && newApplication,
    int64 commitDuration,
    int64 plbDuration,
    ErrorCode error,
    bool isPlbUpdated)
{
    if (!error.IsSuccess())
    {
        if (isPlbUpdated)
        {
            int64 revertPlbDuration;
            // Force update of PLB to revert to current version of application description
            PLBUpdateApplication(*lockedApplication, revertPlbDuration, true);
            //Add the duration to the overall PLB elapsed time
            plbDuration += revertPlbDuration;
        }

        fm_.ServiceEvents.AppUpdateFailure(newApplication->IdString, newApplication->ApplicationId, error, commitDuration, plbDuration);
    }
    else
    {
        fm_.WriteInfo(
            Constants::AppUpdateSource, lockedApplication->IdString,
            "Application updated: {0}\r\nCommit Duration = {1} ms, PLB Duration = {2} ms", *newApplication, commitDuration, plbDuration);

        lockedApplication.Commit(move(newApplication));
    }
}

void ServiceCache::UpdateApplicationAsync(LockedApplicationInfo && lockedAppInfo, ApplicationInfoSPtr && newAppInfo, bool isPlbUpdateNeeded)
{
    BeginUpdateApplication(
        move(lockedAppInfo),
        move(newAppInfo),
        isPlbUpdateNeeded,
        [this](AsyncOperationSPtr const& operation)
        {
            EndUpdateApplication(operation);
        },
        fm_.CreateAsyncOperationRoot());
}

AsyncOperationSPtr ServiceCache::BeginRepartitionService(
    ServiceDescription && serviceDescription,
    vector<ConsistencyUnitDescription> && added,
    vector<ConsistencyUnitDescription> && removed,
    LockedServiceInfo && lockedServiceInfo,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<RepartitionServiceAsyncOperation>(
        *this,
        move(lockedServiceInfo),
        move(serviceDescription),
        move(added),
        move(removed),
        callback,
        parent);
}

ErrorCode ServiceCache::EndRepartitionService(AsyncOperationSPtr const& operation)
{
    return RepartitionServiceAsyncOperation::End(operation);
}

AsyncOperationSPtr ServiceCache::BeginCreateApplication(
    ApplicationIdentifier const & applicationId,
    uint64 instanceId,
    NamingUri const & applicationName,
    uint64 updateId,
    ApplicationCapacityDescription const & capacityDescription,
    ServiceModel::ServicePackageResourceGovernanceMap const& rgDescription,
    ServiceModel::CodePackageContainersImagesMap const& cpContainersImages,
    StringCollection networks,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & state)
{
     ErrorCode error(ErrorCodeValue::Success);

    if (Management::NetworkInventoryManager::NetworkInventoryManagerConfig::IsNetworkInventoryManagerEnabled())
    {
        // Associate the application to requested networks.
        for( auto networkName : networks) 
        {
            ManualResetEvent completedEvent(false);
            fm_.NIS.BeginNetworkAssociation(
                networkName,
                applicationName.ToString(),
                [this, &completedEvent, &error](AsyncOperationSPtr const & contextSPtr) mutable -> void
                {
                    error = fm_.NIS.EndNetworkAssociation(contextSPtr);
                    completedEvent.Set();
                },
                state);

            completedEvent.WaitOne();
            if (!error.IsSuccess())
            {
                break;
            }
        }
    }

    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    LockedApplicationInfo lockedAppInfo;
    bool isNewApplication;
    error = CreateOrGetLockedApplication(
        applicationId,
        applicationName,
        instanceId,
        updateId,
        capacityDescription,
        rgDescription,
        cpContainersImages,
        lockedAppInfo,
        isNewApplication);

    if (!error.IsSuccess() || !isNewApplication)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::ApplicationAlreadyExists, callback, state);
    }

    auto appInfo = lockedAppInfo.Entry->Get();
    int64 plbDuration;
    error = PLBUpdateApplication(*appInfo, plbDuration);
    if (!error.IsSuccess())
    {
        {
            AcquireWriteLock grab(lock_);

            lockedAppInfo.IsDeleted = true;
            applications_.erase(applicationId);
        }

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    appInfo->Networks = networks;
    pair<LockedApplicationInfo, int64> context(move(lockedAppInfo), move(plbDuration));

    return fm_.Store.BeginUpdateData(
        *appInfo,
        OperationContextCallback<pair<LockedApplicationInfo, int64>>(callback, move(context)),
        state);
}

ErrorCode ServiceCache::EndCreateApplication(AsyncOperationSPtr const& operation)
{
    auto context = operation->PopOperationContext<pair<LockedApplicationInfo, int64>>();

    if (context)
    {
        LockedApplicationInfo & lockedAppInfo = context->Context.first;
        int64 & plbDuration = context->Context.second;
        auto appInfo = lockedAppInfo.Entry->Get();

        int64 commitDuration(0);
        ErrorCode error = fm_.Store.EndUpdateData(*appInfo, operation, commitDuration);

        if (error.IsSuccess())
        {
            fm_.WriteInfo(
                Constants::AppUpdateSource, appInfo->IdString,
                "Application added: {0}\r\nCommit Duration = {1} ms, PLB Duration = {2}  ms", *appInfo, commitDuration, plbDuration);
        }
        else
        {
            int64 revertPlbDuration(0);
            // In case of failure, delete application from PLB (forced)
            PLBDeleteApplication(appInfo->ApplicationName, revertPlbDuration, true);
            plbDuration += revertPlbDuration;

            if (Management::NetworkInventoryManager::NetworkInventoryManagerConfig::IsNetworkInventoryManagerEnabled())
            {
                // Disassociate the network from the app in case of failure
                for( auto networkName : appInfo->Networks)
                {        
                    ManualResetEvent completedEvent(false);

                    fm_.NIS.BeginNetworkDissociation(
                        networkName,
                        appInfo->ApplicationName.ToString(),
                        [&completedEvent](AsyncOperationSPtr const&) { completedEvent.Set(); },
                        fm_.CreateAsyncOperationRoot());

                    completedEvent.WaitOne();
                }
            }

            RemoveApplicationCommitJobItemUPtr commitJobItem = make_unique<RemoveApplicationCommitJobItem>(move(lockedAppInfo), commitDuration, plbDuration);
            fm_.CommitQueue.Enqueue(move(commitJobItem));
        }

        return error;
    }

    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr ServiceCache::BeginUpdateApplication(
    ApplicationIdentifier const & applicationId,
    uint64 updateId,
    ApplicationCapacityDescription const& capacityDescription,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    LockedApplicationInfo lockedAppInfo;
    ErrorCode error = GetLockedApplication(applicationId, lockedAppInfo);
    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    if (lockedAppInfo->UpdateId > updateId)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::StaleRequest, callback, state);
    }
    if (lockedAppInfo->IsDeleted)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::ApplicationNotFound, callback, state);
    }

    ApplicationInfoSPtr newAppInfo = make_shared<ApplicationInfo>(*lockedAppInfo);
    newAppInfo->UpdateId = updateId;
    newAppInfo->ApplicationCapacity = capacityDescription;

    return BeginUpdateApplication(
        move(lockedAppInfo),
        move(newAppInfo),
        true, // IsPlbUpdateNeeded
        callback,
        state);
}

AsyncOperationSPtr ServiceCache::BeginUpdateApplication(
    LockedApplicationInfo && lockedAppInfo,
    ApplicationInfoSPtr && newAppInfo,
    bool isPlbUpdateNeeded,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    ErrorCode error(ErrorCodeValue::Success);

    int64 plbDuration(0);

    if (isPlbUpdateNeeded)
    {
        error = PLBUpdateApplication(*newAppInfo, plbDuration);
    }

    if (error.IsSuccess())
    {
        auto newAppInfoPtr = newAppInfo.get();
        tuple<LockedApplicationInfo, ApplicationInfoSPtr, int64> context(move(lockedAppInfo), move(newAppInfo), move(plbDuration));

        return fmStore_.BeginUpdateData(
            *newAppInfoPtr,
            OperationContextCallback<tuple<LockedApplicationInfo, ApplicationInfoSPtr, int64>>(callback, move(context)),
            state);
    }

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
}

ErrorCode ServiceCache::EndUpdateApplication(AsyncOperationSPtr const& operation)
{
    unique_ptr<OperationContext<tuple<LockedApplicationInfo, ApplicationInfoSPtr, int64>>> context = 
        operation->PopOperationContext<tuple<LockedApplicationInfo, ApplicationInfoSPtr, int64>>();

    if (context)
    {
        LockedApplicationInfo & lockedAppInfo = get<0>(context->Context);
        ApplicationInfoSPtr & newAppInfo = get<1>(context->Context);
        int64 & plbDuration = get<2>(context->Context);

        int64 commitDuration(0);
        ErrorCode error = fmStore_.EndUpdateData(*newAppInfo, operation, commitDuration);

        PostUpdateApplication(lockedAppInfo, move(newAppInfo), commitDuration, plbDuration, error, true);

        return error;
    }

    return CompletedAsyncOperation::End(operation);
}

ErrorCode ServiceCache::CreateService(
    ServiceDescription && serviceDescription,
    vector<ConsistencyUnitDescription> && consistencyUnitDescriptions,
    bool isCreateFromRebuild,
    bool isServiceUpdateNeeded)
{
    ManualResetEvent completedEvent(false);

    AsyncOperationSPtr operation = BeginCreateService(
        move(serviceDescription),
        move(consistencyUnitDescriptions),
        isCreateFromRebuild,
        isServiceUpdateNeeded,
        [&completedEvent](AsyncOperationSPtr const&) { completedEvent.Set(); },
        fm_.CreateAsyncOperationRoot());

    completedEvent.WaitOne();

    return EndCreateService(operation);
}

AsyncOperationSPtr ServiceCache::BeginCreateService(
    ServiceDescription && serviceDescription,
    vector<ConsistencyUnitDescription> && consistencyUnitDescriptions,
    bool isCreateFromRebuild,
    bool isServiceUpdateNeeded,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<CreateServiceAsyncOperation>(
        *this,
        move(serviceDescription),
        move(consistencyUnitDescriptions),
        isCreateFromRebuild,
        isServiceUpdateNeeded,
        callback,
        parent);
}

ErrorCode ServiceCache::EndCreateService(AsyncOperationSPtr const& operation)
{
    return CreateServiceAsyncOperation::End(operation);
}

ErrorCode ServiceCache::CreateServiceDuringRebuild(
    ServiceDescription && serviceDescription,
    vector<ConsistencyUnitDescription> && consistencyUnitDescriptions,
    bool isServiceUpdateNeeded)
{
    fm_.ServiceEvents.CreateServiceDuringRebuild(serviceDescription.Name, serviceDescription);

    return CreateService(move(serviceDescription), move(consistencyUnitDescriptions), true, isServiceUpdateNeeded);
}

ErrorCode ServiceCache::CreateTombstoneService()
{
    ErrorCode error(ErrorCodeValue::Success);

    if (!ServiceExists(Constants::TombstoneServiceName))
    {
        error = fm_.ServiceCacheObj.CreateService(
            GetTombstoneServiceDescription(),
            vector<ConsistencyUnitDescription>(),
            true);
    }

    return error;
}

AsyncOperationSPtr ServiceCache::BeginUpdateSystemService(
    wstring && serviceName,
    Naming::ServiceUpdateDescription && serviceUpdateDescription,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    fm_.ServiceEvents.UpdateSystemService(serviceName, serviceUpdateDescription);

    LockedServiceInfo lockedServiceInfo;
    ErrorCode error = GetLockedService(SystemServiceApplicationNameHelper::GetInternalServiceName(serviceName), lockedServiceInfo);
    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    ServiceDescription serviceDescription = lockedServiceInfo->ServiceDescription;

    bool isUpdated = false;
    error = serviceUpdateDescription.TryUpdateServiceDescription(serviceDescription, isUpdated);

    if (error.IsSuccess() && isUpdated)
    {
        ServiceInfoSPtr newServiceInfo = make_shared<ServiceInfo>(*lockedServiceInfo, move(serviceDescription));
        newServiceInfo->IsServiceUpdateNeeded = true;

        return BeginInternalUpdateServiceInfo(
            move(lockedServiceInfo),
            move(newServiceInfo),
            true,
            OperationContextCallback<bool>(callback, false /* IsRepartitionService */),
            state);
    }

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
}

ErrorCode ServiceCache::EndUpdateSystemService(AsyncOperationSPtr const& operation)
{
    return EndInternalUpdateServiceInfo(operation);
}

AsyncOperationSPtr ServiceCache::BeginUpdateService(
    ServiceDescription && serviceDescription,
    vector<ConsistencyUnitDescription> && added,
    vector<ConsistencyUnitDescription> && removed,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    fm_.ServiceEvents.UpdateService(serviceDescription.Name, serviceDescription, added.size(), removed.size());

    LockedServiceInfo lockedServiceInfo;
    ErrorCode error = GetLockedService(serviceDescription.Name, lockedServiceInfo);
    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    if (serviceDescription.Instance > lockedServiceInfo->Instance)
    {
        Assert::TestAssert("Invalid UpdateService: Incoming instance {0} is greater than existing instance {1}.", serviceDescription.Instance, lockedServiceInfo->Instance);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::InstanceIdMismatch, callback, state);
    }
    if (lockedServiceInfo->IsDeleted)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::FMServiceDoesNotExist, callback, state);
    }
    if (lockedServiceInfo->IsToBeDeleted)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::FMServiceDeleteInProgress, callback, state);
    }
    if (serviceDescription.Instance < lockedServiceInfo->Instance)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::StaleRequest, callback, state);
    }
    if (lockedServiceInfo->ServiceDescription.UpdateVersion > serviceDescription.UpdateVersion)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::StaleRequest, callback, state);
    }
    if (lockedServiceInfo->ServiceDescription.UpdateVersion == serviceDescription.UpdateVersion)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::Success, callback, state);
    }

    serviceDescription.PackageVersionInstance = lockedServiceInfo->ServiceDescription.PackageVersionInstance;

    if (added.empty() && removed.empty())
    {
        ServiceInfoSPtr newServiceInfo = make_shared<ServiceInfo>(*lockedServiceInfo, move(serviceDescription));
        newServiceInfo->IsServiceUpdateNeeded = true;

        return BeginInternalUpdateServiceInfo(
            move(lockedServiceInfo),
            move(newServiceInfo),
            true, // IsPlbUpdateNeeded
            OperationContextCallback<bool>(callback, false /* IsRepartitionService */),
            state);
    }
    else
    {
        return BeginRepartitionService(
            move(serviceDescription),
            move(added),
            move(removed),
            move(lockedServiceInfo),
            OperationContextCallback<bool>(callback, true /* IsRepartitionService */),
            state);
    }
}

ErrorCode ServiceCache::EndUpdateService(AsyncOperationSPtr const& operation)
{
    unique_ptr<OperationContext<bool>> context = operation->PopOperationContext<bool>();

    if (context)
    {
        bool isRepartitionService = context->Context;
        if (isRepartitionService)
        {
            return EndRepartitionService(operation);
        }
        else
        {
            return EndInternalUpdateServiceInfo(operation);
        }
    }

    return CompletedAsyncOperation::End(operation);
}

ErrorCode ServiceCache::UpdateService(ServiceDescription && serviceDescription)
{
    ManualResetEvent completedEvent(false);

    AsyncOperationSPtr operation = BeginUpdateService(
        move(serviceDescription),
        vector<ConsistencyUnitDescription>(), // Added
        vector<ConsistencyUnitDescription>(), // Removed
        [&completedEvent](AsyncOperationSPtr const&) { completedEvent.Set(); },
        fm_.CreateAsyncOperationRoot());

    completedEvent.WaitOne();

    return EndUpdateService(operation);
}

void ServiceCache::ProcessNodeUpdateServiceReply(NodeUpdateServiceReplyMessageBody const& body, NodeInstance const& from)
{
    fm_.WriteInfo(
        Constants::ServiceSource, body.ServiceName,
        "Processing NodeUpdateServiceReply message from {0}: {1}", from, body);

    LockedServiceInfo lockedServiceInfo;
    ErrorCode error = GetLockedService(body.ServiceName, lockedServiceInfo);
    if (!error.IsSuccess())
    {
        fm_.WriteWarning(
            Constants::ServiceSource, body.ServiceName,
            "NodeUpdateServiceReply processing for {0} from {1} failed with {2}.", body.ServiceName, from, error);

        return;
    }

    // We keep the service locked so that no other thread would update ServiceUpdateState.
    // Updates need not be persisted, hence the locked service is not committed and updates
    // are made on the existing object.

    ServiceInfoSPtr serviceInfo = GetService(body.ServiceName);

    if (serviceInfo->Instance != body.ServiceInstance ||
        !serviceInfo->IsServiceUpdateNeeded ||
        serviceInfo->ServiceDescription.UpdateVersion != body.ServiceUpdateVersion)
    {
        return;
    }

    serviceInfo->AddUpdatedNode(from.Id);
}

AsyncOperationSPtr ServiceCache::BeginAddFailoverUnitId(
    wstring const& serviceName,
    FailoverUnitId const& failoverUnitId,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    LockedServiceInfo lockedServiceInfo;
    ErrorCode error = GetLockedService(serviceName, lockedServiceInfo);
    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    ServiceInfoSPtr newServiceInfo = make_shared<ServiceInfo>(*lockedServiceInfo);
    newServiceInfo->AddFailoverUnitId(failoverUnitId);

    return BeginInternalUpdateServiceInfo(
        move(lockedServiceInfo),
        move(newServiceInfo),
        false, // IsPlbUpdateNeeded
        callback,
        state);
}

ErrorCode ServiceCache::EndAddFailoverUnitId(AsyncOperationSPtr const& operation)
{
    return EndInternalUpdateServiceInfo(operation);
}

void ServiceCache::SetFailoverUnitIdsAsync(wstring const& serviceName, uint64 serviceInstance, set<FailoverUnitId> && failoverUnitIds)
{
    LockedServiceInfo lockedServiceInfo;
    ErrorCode error = GetLockedService(serviceName, lockedServiceInfo);
    if (!error.IsSuccess())
    {
        fm_.WriteWarning(
            Constants::ServiceSource, serviceName,
            "SetFailoverUnitIdsAsync for service {0} failed with {1}", serviceName, error);

        return;
    }

    if (lockedServiceInfo->Instance != serviceInstance)
    {
        // A new service instance has been created.
        // This is a stale request.
        return;
    }

    ServiceInfoSPtr newServiceInfo = make_shared<ServiceInfo>(*lockedServiceInfo);

    if (!newServiceInfo->SetFailoverUnitIds(move(failoverUnitIds)))
    {
        // No new partition has been discovered
        return;
    }

    auto mover = MoveUPtr<LockedServiceInfo>(move(make_unique<LockedServiceInfo>(move(lockedServiceInfo))));

    fmStore_.BeginUpdateData(
        *newServiceInfo,
        [this, mover, newServiceInfo](AsyncOperationSPtr const& updateOperation) mutable
        {
            int64 commitDuration(0);
            ErrorCode error = fmStore_.EndUpdateData(*newServiceInfo, updateOperation, commitDuration);

            if (error.IsSuccess())
            {
                auto lockedServiceInfo = mover.TakeUPtr();
                InternalUpdateServiceInfoCache(*lockedServiceInfo, move(newServiceInfo), commitDuration, 0);
            }
            else
            {
                fm_.WriteWarning(
                    Constants::ServiceSource, newServiceInfo->Name,
                    "SetFailoverUnitIdsAsync for service {0} failed with {1}", newServiceInfo->Name, error);
            }
        },
        fm_.CreateAsyncOperationRoot());
}

ErrorCode ServiceCache::IterateOverFailoverUnits(wstring const& serviceName, function<ErrorCode(LockedFailoverUnitPtr &)> const& func) const
{
    return IterateOverFailoverUnits(
        serviceName,
        [](FailoverUnitId const&) { return true; },
        func);
}

ErrorCode ServiceCache::IterateOverFailoverUnits(wstring const& serviceName, function<bool(FailoverUnitId const&)> const& filter, function<ErrorCode(LockedFailoverUnitPtr &)> const& func) const
{
    ServiceInfoSPtr serviceInfo = GetService(serviceName);
    if (!serviceInfo)
    {
        return ErrorCodeValue::ServiceNotFound;
    }

    if (serviceInfo->FailoverUnitIds.empty())
    {
        fm_.WriteWarning(
            Constants::ServiceSource,
            serviceName,
            "FailoverUnitIds list is not initialized for service {0}", serviceName);

        return ErrorCodeValue::UpdatePending;
    }

    ErrorCode error(ErrorCodeValue::Success);

    for (FailoverUnitId failoverUnitId : serviceInfo->FailoverUnitIds)
    {
        if (filter(failoverUnitId))
        {
            LockedFailoverUnitPtr failoverUnit;
            if (fm_.FailoverUnitCacheObj.TryGetLockedFailoverUnit(failoverUnitId, failoverUnit))
            {
                if (failoverUnit)
                {
                    ErrorCode e = func(failoverUnit);
                    if (!e.IsSuccess())
                    {
                        error = ErrorCode::FirstError(error, e);
                    }
                }
                else
                {
                    error = ErrorCode::FirstError(error, ErrorCodeValue::PartitionNotFound);
                }
            }
            else
            {
                error = ErrorCode::FirstError(error, ErrorCodeValue::UpdatePending);
            }
        }
    }

    return error;
}

AsyncOperationSPtr ServiceCache::BeginProcessPLBSafetyCheck(
    ServiceModel::ApplicationIdentifier && appId,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    return AsyncOperation::CreateAndStart<ProcessPLBSafetyCheckAsyncOperation>(
        *this,
        move(appId),
        callback,
        state);
}

ErrorCode ServiceCache::EndProcessPLBSafetyCheck(
    AsyncOperationSPtr const& operation)
{
    return ProcessPLBSafetyCheckAsyncOperation::End(operation);
}

AsyncOperationSPtr ServiceCache::BeginProcessUpgrade(
    UpgradeApplicationRequestMessageBody && requestBody,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    return AsyncOperation::CreateAndStart<ProcessUpgradeAsyncOperation>(
        *this,
        move(requestBody),
        callback,
        state);
}

ErrorCode ServiceCache::EndProcessUpgrade(
    AsyncOperationSPtr const& operation,
    __out UpgradeApplicationReplyMessageBody & replyBody)
{
    return ProcessUpgradeAsyncOperation::End(operation, replyBody);
}

void ServiceCache::UpdateUpgradeProgressAsync(
    ApplicationInfo const& appInfo,
    map<NodeId, NodeUpgradeProgress> && pendingNodes,
    map<NodeId, pair<NodeUpgradeProgress, NodeInfoSPtr>> && readyNodes,
    map<NodeId, NodeUpgradeProgress> && waitingNodes,
    bool isUpgradeNeededOnCurrentUD,
    bool isCurrentDomainComplete)
{
    LockedApplicationInfo lockedAppInfo;
    ErrorCode error = GetLockedApplication(appInfo.ApplicationId, lockedAppInfo);

    if (error.IsSuccess())
    {
        if (lockedAppInfo->OperationLSN != appInfo.OperationLSN)
        {
            // The application object has been updated. Ignore stale upgrade progress details.
            return;
        }

        // If all the replica get moved out of a node during upgrade (this can happen for volatile services with
        // TargetReplicaSetSize of 1), no upgrade message is sent to the node. In this case, the node is considered
        // upgraded and we should add the node to the list of completed nodes.
        if (pendingNodes.empty() && readyNodes.empty())
        {
            for (auto const& upgradeProgress : waitingNodes)
            {
                lockedAppInfo->Upgrade->AddCompletedNode(upgradeProgress.first);
            }
        }

        lockedAppInfo->Upgrade->UpdateProgressDetails(move(pendingNodes), move(readyNodes), move(waitingNodes), isCurrentDomainComplete);

        ApplicationUpgradeUPtr newUpgrade = make_unique<ApplicationUpgrade>(*lockedAppInfo->Upgrade);
        ApplicationUpgradeUPtr newRollback = 
            (lockedAppInfo->Rollback ? make_unique<ApplicationUpgrade>(*lockedAppInfo->Rollback) : nullptr);

        bool isUpdated = newUpgrade->UpdateProgress(fm_, readyNodes, isUpgradeNeededOnCurrentUD, isCurrentDomainComplete);
        bool isUpgradeComplete = newUpgrade->IsCompleted();

        if (isUpgradeComplete)
        {
            if (Management::NetworkInventoryManager::NetworkInventoryManagerConfig::IsNetworkInventoryManagerEnabled())
            {
                // Disassociate the networks that are not part of the app anymore.
                auto associatedNetworks = fm_.NIS.GetApplicationNetworkList(lockedAppInfo->ApplicationName.ToString());
                for (const auto & net1 : associatedNetworks)
                {
                    bool found = false;
                    for (const auto & net2 : newUpgrade->Description.Specification.Networks)
                    {
                        if (StringUtility::Compare(net1.NetworkName, net2) == 0) 
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        ManualResetEvent completedEvent(false);

                        fm_.NIS.BeginNetworkDissociation(
                            net1.NetworkName,
                            lockedAppInfo->ApplicationName.ToString(),
                            [&completedEvent](AsyncOperationSPtr const&) { completedEvent.Set(); },
                            fm_.CreateAsyncOperationRoot());

                        completedEvent.WaitOne();
                    }
                }
            }

            lockedAppInfo.Entry->Get()->Networks = newUpgrade->Description.Specification.Networks;

            CompleteUpgradeAsync(lockedAppInfo);
        }
        else if (isCurrentDomainComplete || isUpdated)
        {
            ApplicationInfoSPtr newAppInfo = make_shared<ApplicationInfo>(*lockedAppInfo, move(newUpgrade), move(newRollback));
            UpdateApplicationAsync(move(lockedAppInfo), move(newAppInfo), isCurrentDomainComplete);
        }
    }
}

void ServiceCache::CompleteUpgradeAsync(LockedApplicationInfo & lockedAppInfo)
{
    fm_.WriteInfo(
        FailoverManager::TraceApplicationUpgrade, wformatString(lockedAppInfo->ApplicationId),
        "Completing upgrade for {0}", *lockedAppInfo);

    bool isServiceUpdateNeeded = UpdateServicePackageVersionInstanceAsync(lockedAppInfo);

    if (!isServiceUpdateNeeded)
    {
        ApplicationUpgradeUPtr const& upgrade = lockedAppInfo->Upgrade;
        //we need to update the new rg settings once the upgrade is done
        auto const & upgradedRGSettings = upgrade->Description.Specification.UpgradedRGSettings;
        auto const & upgradedContainersImages = upgrade->Description.Specification.UpgradedCPContainersImages;

        ApplicationInfoSPtr newAppInfo = make_shared<ApplicationInfo>(*lockedAppInfo, nullptr, nullptr);
        newAppInfo->ResourceGovernanceDescription = upgradedRGSettings;
        newAppInfo->CodePackageContainersImages = upgradedContainersImages;

        UpdateApplicationAsync(move(lockedAppInfo), move(newAppInfo), true);
    }
}

bool ServiceCache::UpdateServicePackageVersionInstanceAsync(LockedApplicationInfo & lockedAppInfo)
{
    bool isServiceUpdateNeeded = false;

    AcquireReadLock grab(lock_);

    for (auto it = services_.begin(); it != services_.end(); ++it)
    {
        ServiceInfoSPtr serviceInfo = it->second->Get();

        ServicePackageVersionInstance versionInstance;
        if (lockedAppInfo->GetUpgradeVersionForServiceType(serviceInfo->ServiceType->Type, versionInstance) &&
            serviceInfo->ServiceDescription.PackageVersionInstance != versionInstance)
        {
            LockedServiceInfo lockedServiceInfo;
            ErrorCode error = it->second->Lock(it->second, lockedServiceInfo, TimeSpan::Zero);
            if (!error.IsSuccess())
            {
                fm_.WriteWarning(
                    FailoverManager::TraceApplicationUpgrade, wformatString(lockedAppInfo->ApplicationId),
                    "Unable to lock service {0}", it->first);

                return true;
            }

            if (lockedAppInfo->GetUpgradeVersionForServiceType(lockedServiceInfo->ServiceType->Type, versionInstance) &&
                lockedServiceInfo->ServiceDescription.PackageVersionInstance != versionInstance)
            {
                isServiceUpdateNeeded = true;

                ServiceDescription newServiceDescription(lockedServiceInfo->ServiceDescription, versionInstance);
                ServiceInfoSPtr newServiceInfo = make_shared<ServiceInfo>(*lockedServiceInfo, move(newServiceDescription));

                auto mover = MoveUPtr<LockedServiceInfo>(move(make_unique<LockedServiceInfo>(move(lockedServiceInfo))));

                fmStore_.BeginUpdateData(
                    *newServiceInfo,
                    [this, mover, newServiceInfo](AsyncOperationSPtr const& updateOperation) mutable
                    {
                        auto lockedServiceInfoUPtr = mover.TakeUPtr();
                        LockedServiceInfo & lockedServiceInfo = *lockedServiceInfoUPtr;

                        int64 commitDuration(0);
                        ErrorCode error = fmStore_.EndUpdateData(*newServiceInfo, updateOperation, commitDuration);
                        if (error.IsSuccess())
                        {
                            InternalUpdateServiceInfoCache(lockedServiceInfo, move(newServiceInfo), commitDuration, 0);
                        }
                        else
                        {
                            fm_.ServiceEvents.ServiceUpdateFailure(newServiceInfo->Name, error, commitDuration, 0);
                        }
                    },
                    fm_.CreateAsyncOperationRoot());
            }
        }
    }

    return isServiceUpdateNeeded;
}

ErrorCode ServiceCache::ProcessUpgradeReply(
    ApplicationIdentifier const & applicationId,
    uint64 instanceId,
    NodeInstance const & from)
{
    LockedApplicationInfo lockedApplication;
    ErrorCode error = GetLockedApplication(applicationId, lockedApplication);
    if (error.IsSuccess())
    {
        if (lockedApplication->Upgrade &&
            lockedApplication->Upgrade->Description.InstanceId == instanceId)
        {
            lockedApplication->Upgrade->AddCompletedNode(from.Id);
            fm_.WriteInfo(
                FailoverManager::TraceApplicationUpgrade, wformatString(applicationId),
                "Node {0} completed upgrade for {1} instance {2}",
                from, applicationId, instanceId);
        }
    }

    return error;
}

ErrorCode ServiceCache::GetLockedService(
    wstring const& serviceName,
    __out LockedServiceInfo & lockedServiceInfo) const
{
    ServiceEntrySPtr entry;

    {
        AcquireReadLock grab(lock_);

        auto it = services_.find(serviceName);
        if (it == services_.end())
        {
            return ErrorCodeValue::FMServiceDoesNotExist;
        }

        entry = it->second;
    }

    return entry->Lock(entry, lockedServiceInfo);
}

ErrorCode ServiceCache::CreateOrGetLockedService(
    ServiceDescription const& serviceDescription,
    ServiceTypeSPtr const& serviceType,
    FABRIC_SEQUENCE_NUMBER healthSequence,
    bool isServiceUpdatedNeeded,
    __out LockedServiceInfo & lockedServiceInfo,
    __out bool & isNewService)
{
    isNewService = false;

    ServiceEntrySPtr entry;

    {
        // No need to acquire write lock when the service type already exists
        AcquireReadLock grab(lock_);

        auto it = services_.find(serviceDescription.Name);
        if (it != services_.end())
        {
            entry = it->second;
        }
    }

    if (entry)
    {
        ErrorCode error = entry->Lock(entry, lockedServiceInfo);
        if (!error.IsError(ErrorCodeValue::FMServiceDoesNotExist))
        {
            return error;
        }
    }

    AcquireWriteLock grab(lock_);

    // We need to check again for the existence of the service type because it may have been created by a different thread
    auto it = services_.find(serviceDescription.Name);
    if (it == services_.end())
    {
        ServiceInfoSPtr serviceInfo = make_shared<ServiceInfo>(serviceDescription, serviceType, healthSequence, isServiceUpdatedNeeded);
        entry = make_shared<CacheEntry<ServiceInfo>>(move(serviceInfo));
        it = services_.insert(make_pair(serviceDescription.Name, move(entry))).first;

        isNewService = true;
    }

    // We use timeout of zero when trying to lock service type entry under write lock
    return it->second->Lock(it->second, lockedServiceInfo, TimeSpan::Zero);
}

ErrorCode ServiceCache::GetLockedServiceType(
    ServiceTypeIdentifier const& serviceTypeId,
    __out LockedServiceType & lockedServiceType) const
{
    ServiceTypeEntrySPtr entry;

    {
        AcquireReadLock grab(lock_);

        auto it = serviceTypes_.find(serviceTypeId);
        if (it == serviceTypes_.end())
        {
            return ErrorCodeValue::ServiceTypeNotFound;
        }

        entry = it->second;
    }

    return entry->Lock(entry, lockedServiceType);
}

ErrorCode ServiceCache::CreateOrGetLockedServiceType(
    ServiceTypeIdentifier const& serviceTypeId,
    ApplicationEntrySPtr const& applicationEntry,
    __out LockedServiceType & lockedServiceType,
    __out bool & isNewServiceType)
{
    isNewServiceType = false;

    ServiceTypeEntrySPtr entry;

    {
        // No need to acquire write lock when the service type already exists
        AcquireReadLock grab(lock_);

        auto it = serviceTypes_.find(serviceTypeId);
        if (it != serviceTypes_.end())
        {
            entry = it->second;
        }
    }

    if (entry)
    {
        ErrorCode error = entry->Lock(entry, lockedServiceType);
        if (!error.IsError(ErrorCodeValue::ServiceTypeNotFound))
        {
            return error;
        }
    }

    AcquireWriteLock grab(lock_);

    // We need to check again for the existence of the service type because it may have been created by a different thread
    auto it = serviceTypes_.find(serviceTypeId);
    if (it == serviceTypes_.end())
    {
        ServiceTypeSPtr serviceType = make_shared<ServiceType>(serviceTypeId, applicationEntry);
        entry = make_shared<CacheEntry<ServiceType>>(move(serviceType));
        it = serviceTypes_.insert(make_pair(serviceTypeId, move(entry))).first;

        isNewServiceType = true;
    }

    // We use timeout of zero when trying to lock service type entry under write lock
    return it->second->Lock(it->second, lockedServiceType, TimeSpan::Zero);
}

ErrorCode ServiceCache::GetLockedApplication(
    ApplicationIdentifier const& applicationId,
    __out LockedApplicationInfo & lockedApplication) const
{
    ApplicationEntrySPtr entry;

    {
        AcquireReadLock grab(lock_);

        auto it = applications_.find(applicationId);
        if (it == applications_.end())
        {
            return ErrorCodeValue::ApplicationNotFound;
        }

        entry = it->second;
    }

    return entry->Lock(entry, lockedApplication);
}

ErrorCode ServiceCache::CreateOrGetLockedApplication(
    ApplicationIdentifier const& applicationId,
    NamingUri const& applicationName,
    uint64 instanceId,
    uint64 updateId,
    ApplicationCapacityDescription const& capacityDescription,
    ServiceModel::ServicePackageResourceGovernanceMap const& rgDescription,
    ServiceModel::CodePackageContainersImagesMap const& cpContainersImages,
    __out LockedApplicationInfo & lockedApplication,
    __out bool & isNewApplication)
{
    isNewApplication = false;

    ApplicationEntrySPtr entry;

    {
        // No need to acquire write lock when the application already exists
        AcquireReadLock grab(lock_);

        auto it = applications_.find(applicationId);
        if (it != applications_.end())
        {
            entry = it->second;
        }
    }

    if (entry)
    {
        ErrorCode error = entry->Lock(entry, lockedApplication);
        if (!error.IsError(ErrorCodeValue::ApplicationNotFound))
        {
            return error;
        }
    }

    AcquireWriteLock grab(lock_);

    // We need to check again for the existence of the application because it may have been created by a different thread
    auto it = applications_.find(applicationId);
    if (it == applications_.end())
    {
        ApplicationInfoSPtr applicationInfo = make_shared<ApplicationInfo>(
            applicationId,
            applicationName,
            instanceId,
            updateId,
            capacityDescription,
            rgDescription,
            cpContainersImages);
        entry = make_shared<CacheEntry<ApplicationInfo>>(move(applicationInfo));
        it = applications_.insert(make_pair(applicationId, move(entry))).first;

        isNewApplication = true;
    }

    // We use timeout of zero when trying to lock application entry under write lock
    return it->second->Lock(it->second, lockedApplication, TimeSpan::Zero);
}

bool ServiceCache::GetTargetVersion(
    ServicePackageIdentifier const & packageId,
    wstring const & upgradeDomain,
    ServicePackageVersionInstance & versionInstance) const
{
    AcquireReadLock grab(lock_);

    for (auto it = services_.begin(); it != services_.end(); ++it)
    {
        ServiceInfoSPtr service = it->second->Get();
        if (!service->IsDeleted && service->ServiceType->Type.ServicePackageId == packageId)
        {
            versionInstance = service->GetTargetVersion(upgradeDomain);
            return true;
        }
    }

    return false;
}

FABRIC_SEQUENCE_NUMBER ServiceCache::GetNextHealthSequence()
{
    return InterlockedIncrement64(&healthSequence_) - 1;
}

ErrorCode ServiceCache::PLBUpdateService(ServiceInfo const& serviceInfo, int64 & plbElapsedMilliseconds, bool forceUpdate)
{
    ErrorCode error(ErrorCodeValue::Success);

    if (serviceInfo.Name != Constants::TombstoneServiceName)
    {
        error = plb_.UpdateService(serviceInfo.GetPLBServiceDescription(), plbElapsedMilliseconds, forceUpdate);
    }

    return error;
}

void ServiceCache::PLBDeleteService(wstring const& serviceName, int64 & plbElapsedMilliseconds)
{
    if (serviceName != Constants::TombstoneServiceName)
    {
        plb_.DeleteService(serviceName, plbElapsedMilliseconds);
    }
}

void ServiceCache::PLBUpdateServiceType(ServiceType const& serviceType, int64 & plbElapsedMilliseconds)
{
    if (serviceType.Type.ServiceTypeName != Constants::TombstoneServiceType)
    {
        plb_.UpdateServiceType(serviceType.GetPLBServiceTypeDescription(), plbElapsedMilliseconds);
    }
}

void ServiceCache::PLBDeleteServiceType(wstring const& serviceTypeName, int64 & plbElapsedMilliseconds)
{
    if (serviceTypeName != Constants::TombstoneServiceType)
    {
        plb_.DeleteServiceType(serviceTypeName, plbElapsedMilliseconds);
    }
}

ErrorCode ServiceCache::PLBUpdateApplication(ApplicationInfo & appInfo, int64 & plbElapsedMilliseconds, bool forceUpdate)
{
    ErrorCode error(ErrorCodeValue::Success);

    if (appInfo.ApplicationName != NamingUri())
    {
        // If application does not have a name, it means that it was created prior to 4.3 and never upddated with PLB capacities after that
        // In that case, we do not want to pass it to PLB.
        error = plb_.UpdateApplication(appInfo.GetPLBApplicationDescription(), plbElapsedMilliseconds, forceUpdate);
    }

    return error;
}

void ServiceCache::PLBDeleteApplication(NamingUri const& applicationName, int64 & plbElapsedMilliseconds, bool forceUpdate)
{
    if (applicationName != NamingUri())
    {
        plb_.DeleteApplication(applicationName.ToString(), plbElapsedMilliseconds, forceUpdate);
    }
}

void ServiceCache::AddServiceContexts() const
{
    fm_.Events.PeriodicTaskBeginNoise(fm_.Id, PeriodicTaskName::AddServiceContexts);
    auto startTime = Stopwatch::Now();

    ServiceContextCounts contextCounts;

    map<wstring, uint64> toBeDeletedServices;

    {
        AcquireReadLock grab(lock_);

        for (auto it = services_.begin(); it != services_.end(); it++)
        {
            ServiceInfoSPtr serviceInfo = it->second->Get();

            if (serviceInfo->IsDeleted || serviceInfo->Name == Constants::TombstoneServiceName)
            {
                continue;
            }

            bool partitionCountsDoNotMatch = serviceInfo->FailoverUnitIds.size() != serviceInfo->ServiceDescription.PartitionCount;

            // LSN of zero implies that the service has not been persisted yet
            bool shouldAddToPartitionMapContext = partitionCountsDoNotMatch && serviceInfo->OperationLSN > 0 && !serviceInfo->RepartitionInfo;

            if (partitionCountsDoNotMatch)
            {
                fm_.WriteWarning(
                    Constants::ServiceSource, serviceInfo->Name,
                    "Size of FailoverUnitIds {0} does not match the partition count {1}: {2}",
                    serviceInfo->FailoverUnitIds.size(), serviceInfo->ServiceDescription.PartitionCount, *serviceInfo);
            }

            if (shouldAddToPartitionMapContext)
            {
                fm_.BackgroundManagerObj.AddThreadContext(
                    make_unique<ServiceToPartitionMapContext>(serviceInfo->Name, serviceInfo->Instance));
                contextCounts.ServiceToPartitionMapContexts++;
            }
            
            if (serviceInfo->IsToBeDeleted && (!partitionCountsDoNotMatch || serviceInfo->IsForceDelete))
            {
                toBeDeletedServices[serviceInfo->Name] = serviceInfo->Instance;
            } 
            else if (serviceInfo->IsServiceUpdateNeeded)
            {
                fm_.BackgroundManagerObj.AddThreadContext(make_unique<UpdateServiceContext>(serviceInfo));
                contextCounts.UpdateServiceContexts++;
            }
        }
    }

    for (auto it = toBeDeletedServices.begin(); it != toBeDeletedServices.end(); it++)
    {
        ErrorCode error = fm_.InBuildFailoverUnitCacheObj.DeleteInBuildFailoverUnitsForService(it->first);
        if (error.IsSuccess())
        {
            fm_.BackgroundManagerObj.AddThreadContext(make_unique<DeleteServiceContext>(it->first, it->second));

            contextCounts.DeleteServiceContexts++;
        }
    }

    auto duration = Stopwatch::Now() - startTime;
    fm_.Events.ServiceContextsPeriodicTaskEnd(fm_.Id, duration, contextCounts);
}

void ServiceCache::AddApplicationUpgradeContexts(vector<BackgroundThreadContextUPtr> & contexts) const
{
    AcquireReadLock grab(lock_);

    for (auto it = applications_.begin(); it != applications_.end(); ++it)
    {
        ApplicationInfoSPtr const& appInfo = it->second->Get();

        if (appInfo->Upgrade)
        {
            // Lock the ApplicationInfo to ensure that there is no commit going on. This is because we want to create
            // the ApplicationUpgradeContext from the latest ApplicationInfo. There are two reasons for this:
            //
            // (1) When completing the upgrade context, we ignore the context result if the OperationLSN of the
            //     ApplicationInfo in the context doesn't match the latest OperationLSN (i.e., ApplicationInfo has been
            //     updated). So, there is no point create the upgrade when we know that it will not be accepted later.
            //
            // (2) When creating new replicas, we need to ensure that we are creating the replicas in the correct
            //     version. If we don't have the latest ApplicationInfo, then we may end up creating a replica in a
            //     lower version if the upgrade is moving from one UD to the next around the same time.

            LockedApplicationInfo lockedAppInfo;
            ErrorCode error = it->second->Lock(it->second, lockedAppInfo, TimeSpan::Zero);
            if (error.IsSuccess())
            {
                if (lockedAppInfo->Upgrade)
                {
                    if (lockedAppInfo->Upgrade->UpgradeDomains->IsStale)
                    {
                        auto sortPolicy = lockedAppInfo->Upgrade->UpgradeDomainSortPolicy;
                        UpgradeDomainsCSPtr const& upgradeDomains = fm_.NodeCacheObj.GetUpgradeDomains(sortPolicy);

                        ApplicationInfoSPtr newAppInfo = make_shared<ApplicationInfo>(*lockedAppInfo);
                        newAppInfo->Upgrade->UpdateDomains(upgradeDomains);

                        newAppInfo->AddThreadContext(contexts, newAppInfo);

                        lockedAppInfo.Commit(move(newAppInfo));
                    }
                    else
                    {
                        lockedAppInfo->AddThreadContext(contexts, lockedAppInfo.Get());
                    }
                }
            }
        }
    }
}

ServiceDescription ServiceCache::GetFMServiceDescription()
{
    FailoverConfig & config = FailoverConfig::GetConfig();

    return ServiceDescription(
        SystemServiceApplicationNameHelper::InternalFMServiceName,
        1,      /* Instance */
        0,      /* UpdateVersion */
        1,      /* PartitionCount */
        config.TargetReplicaSetSize,
        config.MinReplicaSetSize,
        true,   /* IsStateful */
        true,   /* HasPersistedState */
        config.ReplicaRestartWaitDuration,
        config.FullRebuildWaitDuration,
        config.StandByReplicaKeepDuration,
        *ServiceTypeIdentifier::FailoverManagerServiceTypeId,
        vector<ServiceCorrelationDescription>(),
        config.PlacementConstraints,
        0,      /* ScaleoutCount */
        vector<ServiceLoadMetricDescription>(),
        FABRIC_MOVE_COST_LOW,      /* DefaultMoveCost */
        vector<byte>());
}

ServiceDescription ServiceCache::GetTombstoneServiceDescription()
{
    return ServiceDescription(
        Constants::TombstoneServiceName,
        0,     /* Instance */
        0,     /* UpdateVersion */
        1,     /* PartitionCount */
        1,     /* TargetReplicaSetSize */
        1,     /* MinReplicaSetSize */
        true,  /* IsStateful */
        true,  /* HasPeristedState */
        ServiceModelConfig::GetConfig().SystemReplicaRestartWaitDuration,
        TimeSpan::MaxValue,
        ServiceModelConfig::GetConfig().SystemStandByReplicaKeepDuration,
        *ServiceTypeIdentifier::TombStoneServiceTypeId,
        vector<ServiceCorrelationDescription>(),
        L"",   /* PlacementConstraints */
        1,     /* ScaleoutCount */
        vector<ServiceLoadMetricDescription>(),
        FABRIC_MOVE_COST_LOW,     /* DefaultMoveCost */
        vector<byte>());
}

void ServiceCache::TraceServices()
{
    AcquireReadLock grab(lock_);

    size_t numToTrace = min(static_cast<size_t>(FailoverConfig::GetConfig().PeriodicServiceTraceCount), services_.size());
    if (numToTrace == 0)
    {
        return;
    }

    nextServiceTraceIndex_ %= services_.size();
    auto it = services_.begin();
    advance(it, nextServiceTraceIndex_); // Set iterator to where we will begin tracing

    for (size_t i = 0; i < numToTrace; i++)
    {
        ServiceInfoSPtr serviceInfo = it->second->Get();
        fm_.ServiceEvents.ServiceState(serviceInfo->Name, *serviceInfo);
        it++;
        if (it == services_.end()) // Wrap around
        {
            it = services_.begin();
        }
    }
    nextServiceTraceIndex_ += numToTrace;
}
