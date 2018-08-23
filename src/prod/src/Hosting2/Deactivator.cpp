// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Management;

StringLiteral const TraceType("Deactivator");

// ********************************************************************************************************************
// Deactivator::ServicePackageInstanceReplicaCountEntry Implementation
//
class Deactivator::ServicePackageInstanceReplicaCountEntry :
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    ServicePackageInstanceReplicaCountEntry(
        ServicePackageInstanceIdentifier const & servicePackageInstanceId, wstring const & traceId)
        : id_(servicePackageInstanceId),
        servicePackageUsageCount_(0),
        closing_(false),
        servicePackageSequenceNumber_(0),
        continuousDeactivationFailureCount_(0),
        servicePackageInstanceUnusedSince_(DateTime::Now()),
        traceId_(traceId),
        entryLock_()
    {
    }

public:
    ErrorCode IncrementUsageCount(Deactivator & deactivator)
    {
        bool shouldCancelDeactivation = false;
        uint64 sequenceNumber = 0;

        {
            AcquireWriteLock writeLock(entryLock_);
            if(closing_) { return ErrorCode(ErrorCodeValue::InvalidState); }

            sequenceNumber = ++this->servicePackageSequenceNumber_;
            if(++this->servicePackageUsageCount_ == 1)
            {
                shouldCancelDeactivation = true;
                continuousDeactivationFailureCount_ = 0;
                this->servicePackageInstanceUnusedSince_ = DateTime::MaxValue;
            }

            WriteInfo(
                TraceType,
                traceId_,
                "IncrementUsageCount: {0}. ShouldCancelDeactivation={1}",                            
                this->ToString(),
                shouldCancelDeactivation);
        }        

        if(shouldCancelDeactivation)
        {
            deactivator.CancelServicePackageInstanceDeactivation(this->id_, sequenceNumber);
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    void DecrementUsageCount(Deactivator & deactivator)
    {
        bool shouldScheduleDeactivation = false;
        uint64 sequenceNumber = 0;

        {
            AcquireWriteLock writeLock(entryLock_);

            ASSERT_IF(closing_, "Decrement call cannot be called in closing state. The usage count is already 0. {0}", this->ToString());

            ASSERT_IF(
                this->servicePackageUsageCount_ - 1 < 0, 
                "DecrementUsageCount: UsageCount cannot be negative. ServicePackage={0}", id_);

            sequenceNumber = ++this->servicePackageSequenceNumber_;
            if(--this->servicePackageUsageCount_ == 0)
            {
                shouldScheduleDeactivation = true;
                this->servicePackageInstanceUnusedSince_ = DateTime::Now();
            }     

            WriteInfo(
                TraceType,
                traceId_,
                "DecrementUsageCount: {0}. ShouldScheduleDeactivation={1}",
                this->ToString(),
                shouldScheduleDeactivation);
        }

        if(shouldScheduleDeactivation)
        {
            this->ScheduleServicePackageInstanceDeactivation(deactivator, sequenceNumber);
        }        
    }

    LONG GetUsageCount()
    {
        {
            AcquireReadLock readLock(entryLock_);
            return this->servicePackageUsageCount_;
        }
    }

    ErrorCode ScheduleDeactivationIfNotUsed(Deactivator & deactivator, bool & isDeactivationScheduled)
    {        
        uint64 sequenceNumber = 0;

        {
            AcquireReadLock readLock(entryLock_);
            if(closing_) { return ErrorCode(ErrorCodeValue::InvalidState); }

            if(this->servicePackageUsageCount_ == 0 && 
               this->GetDeactivationGraceInterval() != TimeSpan::MaxValue)
            {
                sequenceNumber = this->servicePackageSequenceNumber_;
                isDeactivationScheduled = true;
            }
            else
            {
                isDeactivationScheduled = false;
            }

            WriteInfo(
                TraceType,
                traceId_,
                "ScheduleDeactivationIfNotUsed: {0}, IsDeactivationScheduled={1}",
                this->ToString(),
                isDeactivationScheduled);
        }

        if(isDeactivationScheduled)
        {
            this->ScheduleServicePackageInstanceDeactivation(deactivator, sequenceNumber);
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    bool OnDeactivateIfNotUsed(uint64 const sequenceNumber)
    {
        AcquireWriteLock writeLock(entryLock_);
        if(closing_) { return false; }        

        WriteInfo(
            TraceType,
            traceId_,
            "OnDeactivateIfNotUsed: {0}, IncomingSequenceNumber={1}",            
            this->ToString(),
            sequenceNumber);

        if(this->servicePackageSequenceNumber_ == sequenceNumber && this->servicePackageUsageCount_ == 0)
        {
            this->closing_ = true;            
            return true;
        }

        return false;
    }

    void ScheduleDeactivationIfNotUsedSince(DateTime lowerBound, Deactivator & deactivator)
    {
        bool shouldScheduleDeactivation = false;
        uint64 sequenceNumber = 0;
        
        {
            AcquireReadLock readLock(entryLock_);
            if(this->closing_) { return; }

            if(this->servicePackageUsageCount_ == 0 && this->servicePackageInstanceUnusedSince_.Ticks <= lowerBound.Ticks)
            {               
                shouldScheduleDeactivation = true;
                sequenceNumber = this->servicePackageSequenceNumber_;            
            }

            WriteNoise(
                TraceType,
                traceId_,
                "ScheduleDeactivationIfNotUsedSince: ServicePackage={0}, ServicePackageUsageCount={1}, LowerBound={2}, shouldScheduleDeactivation={3}.",
                this->id_,
                this->servicePackageUsageCount_,
                lowerBound,
                shouldScheduleDeactivation);
        }

        if(shouldScheduleDeactivation)
        {
            this->ScheduleServicePackageInstanceDeactivation(deactivator, sequenceNumber);
        }
    }

    void ScheduleServicePackageInstanceDeactivation(
        Deactivator & deactivator,
        uint64 sequenceNumber)
    {
        auto dueTime = GetDeactivationGraceInterval();

        deactivator.ScheduleServicePackageInstanceDeactivation(
            this->id_,
            sequenceNumber,
            dueTime);
    }

    void OnDeactivationFailed(Deactivator & deactivator)
    {
        uint64 sequenceNumber = 0;
        int64 failureCount = 0;
        {
            AcquireWriteLock writeLock(entryLock_);
            ASSERT_IFNOT(closing_, "Attempted to deactivate a ServicePackage without marking it for closing");

            closing_ = false;
            sequenceNumber = this->servicePackageSequenceNumber_;
            failureCount = ++continuousDeactivationFailureCount_;

            WriteInfo(
                TraceType,
                traceId_,
                "OnDeactivationFailed: {0}. ContinuousDeactivationFailureCount={1}",
                this->ToString(),
                this->continuousDeactivationFailureCount_);
        }

        auto deactivationGraceInterval = HostingConfig::GetConfig().DeactivationGraceInterval;
        auto deactivationMaxRetryInterval = HostingConfig::GetConfig().DeactivationMaxRetryInterval;

        auto retryTime = TimeSpan::FromMilliseconds((double)failureCount * deactivationGraceInterval.TotalMilliseconds());

        if (retryTime.Ticks > deactivationMaxRetryInterval.Ticks)
        {
            retryTime = deactivationMaxRetryInterval;
        }

        WriteInfo(
            TraceType,
            traceId_,
            "OnDeactivationFailed: {0}. Scheduled To Run after={1} milliseconds",
            this->ToString(),
            retryTime.TotalMilliseconds());

        deactivator.ScheduleServicePackageInstanceDeactivation(this->id_, sequenceNumber, retryTime);
    }

    TimeSpan GetDeactivationGraceInterval()
    {
        return (id_.ActivationContext.IsExclusive ?
            HostingConfig::GetConfig().ExclusiveModeDeactivationGraceInterval :
            HostingConfig::GetConfig().DeactivationGraceInterval);
    }

    bool IsClosing()
    {
        AcquireReadLock readLock(entryLock_);
        return closing_;
    }

    wstring ToString()
    {
        return wformatString(
            "ServicePackageInstanceReplicaCountEntry: ServicePackageInstanceId={0}, UsageCount={1}, SequenceNumber={2}",
            this->id_,
            this->servicePackageUsageCount_,
            this->servicePackageSequenceNumber_);
    }

private:
    ServicePackageInstanceIdentifier const id_;
    wstring const traceId_;
    RwLock entryLock_;

    /* These members should always be accessed under lock */
    bool closing_;
    LONG servicePackageUsageCount_;
    DateTime servicePackageInstanceUnusedSince_;
    uint64  servicePackageSequenceNumber_;
    int64 continuousDeactivationFailureCount_;
};

// ********************************************************************************************************************
// Deactivator::ApplicationReplicaCountEntry Implementation
//
class Deactivator::ApplicationReplicaCountEntry : private TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    ApplicationReplicaCountEntry(ApplicationIdentifier const & applicationId, wstring const & traceId)
        : id_(applicationId),
        traceId_(traceId),
        closing_(false),
        applicationUsageCount_(0),
        applicationSequenceNumber_(0),
        applicationUnusedSince_(DateTime::Now())
    {
    }

    ErrorCode EnsureServicePackageInstanceEntry(ServicePackageInstanceIdentifier const & servicePackageInstanceId)
    {        
        AcquireWriteLock writeLock(entryLock_);
        if (closing_) { return ErrorCode(ErrorCodeValue::InvalidState); }

        auto iter = servicePackageInstancesMap_.find(servicePackageInstanceId);
        if (iter != servicePackageInstancesMap_.end())
        {
            if(iter->second->IsClosing())
            {
                return ErrorCode(ErrorCodeValue::InvalidState);
            }            
        }
        else
        {            
            servicePackageInstancesMap_.insert(make_pair(servicePackageInstanceId, make_shared<ServicePackageInstanceReplicaCountEntry>(servicePackageInstanceId, traceId_)));

            WriteInfo(
                TraceType,
                traceId_,
                "EnsureServicePackage: New entry with ServicePackage={0} inserted.",
                servicePackageInstanceId);
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode IncrementUsageCount(ServicePackageInstanceIdentifier const & servicePackageInstanceId, Deactivator & deactivator)
    {
        Deactivator::ServicePackageInstanceReplicaCountEntrySPtr ServicePackageInstanceReplicaCountEntry;
        ErrorCode error = GetServicePackageEntry(servicePackageInstanceId, ServicePackageInstanceReplicaCountEntry);

        if(error.IsSuccess())
        {            
            error = ServicePackageInstanceReplicaCountEntry->IncrementUsageCount(deactivator);
            if(error.IsSuccess())
            {
                // If increment on ServicePackage is successful, increment application UsageCount
                bool shouldCancelDeactivation = false;
                uint64 sequenceNumber = 0;

                {
                    // If ServicePackage increment passes and if application is in closing state,
                    // return InvalidState. Application deactivation always succeeds, hence the application
                    // entry will be remove
                    AcquireWriteLock writeLock(entryLock_);
                    if(closing_) { return ErrorCode(ErrorCodeValue::InvalidState); }

                    sequenceNumber = ++this->applicationSequenceNumber_;
                    if(++this->applicationUsageCount_ == 1)
                    {
                        shouldCancelDeactivation = true;
                        this->applicationUnusedSince_ = DateTime::MaxValue;
                    }
                }

                if(shouldCancelDeactivation)
                {
                    deactivator.CancelApplicationDeactivation(this->id_, sequenceNumber);
                }
            }            
        }

        return error;
    }

    ErrorCode DecrementUsageCount(ServicePackageInstanceIdentifier const & servicePackageInstanceId, Deactivator & deactivator)
    {
        Deactivator::ServicePackageInstanceReplicaCountEntrySPtr servicePackageReplicaCountEntry;
        ErrorCode error = GetServicePackageEntry(servicePackageInstanceId, servicePackageReplicaCountEntry);

        if(error.IsSuccess())
        {
            {
                AcquireWriteLock writeLock(entryLock_);                
                if(closing_) { return ErrorCode(ErrorCodeValue::InvalidState); }

                ++this->applicationSequenceNumber_;
                if(--this->applicationUsageCount_ == 0)
                {
                    this->applicationUnusedSince_ = DateTime::Now();
                    // We are not scheduling deactivation of the applicaiton here.
                    // When the last ServicePackage is successfully deactivated,
                    // we will schedule application deactivation.
                }
            }

            servicePackageReplicaCountEntry->DecrementUsageCount(deactivator);
        }

        return error;
    }

    ErrorCode ScheduleDeactivationIfNotUsed(ServicePackageInstanceIdentifier const & servicePackageInstanceId, Deactivator & deactivator, bool & isDeactivationScheduled)
    {
        Deactivator::ServicePackageInstanceReplicaCountEntrySPtr servicePackageReplicaCountEntry;
        ErrorCode error = GetServicePackageEntry(servicePackageInstanceId, servicePackageReplicaCountEntry);

        if(error.IsSuccess())
        {
            return servicePackageReplicaCountEntry->ScheduleDeactivationIfNotUsed(deactivator, isDeactivationScheduled);
        }        

        return error;
    }

    ErrorCode ScheduleDeactivationIfNotUsed(Deactivator & deactivator, bool & isDeactivationScheduled)
    {
        uint64 sequenceNumber = 0;

        {
            AcquireReadLock readLock(entryLock_);
            if(this->closing_) { return ErrorCode(ErrorCodeValue::InvalidState); }

            if(this->applicationUsageCount_ == 0 && 
               HostingConfig::GetConfig().DeactivationGraceInterval != TimeSpan::MaxValue)
            {
                isDeactivationScheduled = true;
                sequenceNumber = this->applicationSequenceNumber_;
            }
            else
            {
                isDeactivationScheduled = false;
            }

            WriteInfo(
                TraceType,
                traceId_,
                "ScheduleDeactivationIfNotUsed: Application={0}, ApplicationUsageCount={1}, IsDeactivationScheduled={2}.",
                this->id_,
                this->applicationUsageCount_,
                isDeactivationScheduled);
        }
        
        if(isDeactivationScheduled)
        {
            deactivator.ScheduleApplicationDeactivation(this->id_, sequenceNumber);
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode ScheduleDeactivationIfNotUsedSince(DateTime lowerBound, Deactivator & deactivator)
    {
        bool shouldScheduleApplicationDeactivation = false;
        uint64 sequenceNumber = 0;

        map<ServicePackageInstanceIdentifier, Deactivator::ServicePackageInstanceReplicaCountEntrySPtr> map;    
        {
            AcquireReadLock readLock(entryLock_);
            if(this->closing_) { return ErrorCode(ErrorCodeValue::InvalidState); }

            if(this->applicationUsageCount_ == 0 && 
                this->applicationUnusedSince_.Ticks <= lowerBound.Ticks)
            {                               
                shouldScheduleApplicationDeactivation = true;
                sequenceNumber = this->applicationSequenceNumber_;                
            }
            else
            {
                map = servicePackageInstancesMap_;
            }

            WriteNoise(
                TraceType,
                traceId_,
                "ScheduleDeactivationIfNotUsedSince: Application={0}, ApplicationUsageCount={1}, LowerBound={2}, shouldScheduleApplicationDeactivation={3}.",
                this->id_,
                this->applicationUsageCount_,
                lowerBound,
                shouldScheduleApplicationDeactivation);
        }

        if(shouldScheduleApplicationDeactivation)
        {
            deactivator.ScheduleApplicationDeactivation(this->id_, sequenceNumber);
        }
        else
        {
            for(auto iter = map.begin(); iter != map.end(); ++iter)
            {
                iter->second->ScheduleDeactivationIfNotUsedSince(lowerBound, deactivator);
            }
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    bool OnDeactivateIfNotUsed(ServicePackageInstanceIdentifier const & servicePackageInstanceId, uint64 const servicePackageSequenceNumber)
    {        
        Deactivator::ServicePackageInstanceReplicaCountEntrySPtr servicePackageReplicaCountEntry;
        ErrorCode error = GetServicePackageEntry(servicePackageInstanceId, servicePackageReplicaCountEntry);

        if(error.IsSuccess())
        {
            return servicePackageReplicaCountEntry->OnDeactivateIfNotUsed(servicePackageSequenceNumber);
        }

        return false;
    }

    bool OnDeactivateIfNotUsed(uint64 const applicationSequenceNumber)
    {
        bool isNotUsed = false;

        {
            AcquireWriteLock writeLock(entryLock_);
            if(closing_) { return false; }

            if(this->applicationUsageCount_ == 0 && applicationSequenceNumber == this->applicationSequenceNumber_)
            {
                closing_ = true;
                isNotUsed = true;
            }

            WriteInfo(
                TraceType,
                traceId_,
                "OnDeactivateIfNotUsed: IsNotUsed={0}, Closing={1}, Application={2}, ApplicationUsageCount={3}.",
                isNotUsed,
                closing_,
                id_,
                this->applicationUsageCount_);
        }

        return isNotUsed;        
    }

    void OnDeactivationFailed(ServicePackageInstanceIdentifier const & servicePackageInstanceId, Deactivator & deactivator)
    {
        Deactivator::ServicePackageInstanceReplicaCountEntrySPtr servicePackageReplicaCountEntry;
        ErrorCode error = GetServicePackageEntry(servicePackageInstanceId, servicePackageReplicaCountEntry);

        if(error.IsSuccess())
        {
            servicePackageReplicaCountEntry->OnDeactivationFailed(deactivator);
        }        
    }

    void OnDeactivationSuccess(ServicePackageInstanceIdentifier const & servicePackageInstanceId, Deactivator & deactivator)
    {
        bool shouldScheduleApplicationDeactivation = false;
        uint64 sequenceNumber = 0;

        {
            AcquireWriteLock writeLock(entryLock_);
            if (closing_) { return; }

            auto iter = servicePackageInstancesMap_.find(servicePackageInstanceId);
            if (iter != servicePackageInstancesMap_.end())
            {
                ASSERT_IFNOT(iter->second->IsClosing(), "Deactivating a ServicePackage without marking it for closing.");

                servicePackageInstancesMap_.erase(iter);

                WriteInfo(
                    TraceType,
                    traceId_,
                    "OnDeactivationSuccess: Entry with ServicePackage={0} removed. Remaining ServicePackage count:{1}.",                            
                    servicePackageInstanceId,
                    servicePackageInstancesMap_.size());

                // If the last ServicePackage is deactivated, schedule the application for deactivation
                if(servicePackageInstancesMap_.empty())
                {                    
                    ASSERT_IF(this->applicationUsageCount_ != 0, "If there are no ServicePackages, the UsageCount has to be 0.");

                    shouldScheduleApplicationDeactivation = true;
                    sequenceNumber = this->applicationSequenceNumber_;
                }
            }
        }

        if(shouldScheduleApplicationDeactivation)
        {
            deactivator.ScheduleApplicationDeactivation(this->id_, sequenceNumber);
        }
    }

    bool DoesServicePackageInstanceEntryExists(ServicePackageInstanceIdentifier const & servicePackageInstanceId, __out LONG & servicePackageReplicaCount)
    {
        Deactivator::ServicePackageInstanceReplicaCountEntrySPtr servicePackageReplicaCountEntry;
        ErrorCode error = GetServicePackageEntry(servicePackageInstanceId, servicePackageReplicaCountEntry);
        if(error.IsSuccess())
        {
            servicePackageReplicaCount = servicePackageReplicaCountEntry->GetUsageCount();
        }

        return error.IsSuccess();
    }

    void GetServicePackageInstanceEntries(
        ServicePackageIdentifier const & servicePackageId,
        __out vector<ServicePackageInstanceIdentifier> & servicePackageInstances)
    {
        {
            AcquireReadLock readLock(entryLock_);
            if (closing_)
            {
                WriteNoise(
                    TraceType,
                    traceId_,
                    "GetServicePackageInstanceEntries: Looking for ServicePackage={0}. ApplicationReplicaCountEntry is closing.",
                    servicePackageId);

                return; 
            }

            for (auto const & packageInstanceEntry : servicePackageInstancesMap_)
            {
                if (packageInstanceEntry.first.IsActivationOf(servicePackageId))
                {
                    servicePackageInstances.push_back(packageInstanceEntry.first);
                }
            }
        }

        WriteNoise(
            TraceType,
            traceId_,
            "GetServicePackageInstanceEntries: Looking for ServicePackage={0}. Number of instances found={1}",
            servicePackageId,
            servicePackageInstances.size());
    }

    LONG GetUsageCount()
    {
        AcquireReadLock readLock(entryLock_);
        return applicationUsageCount_;
    }

    bool IsClosing()
    {
        AcquireReadLock readLock(entryLock_);
        return closing_;
    }

private:
    ErrorCode GetServicePackageEntry(ServicePackageInstanceIdentifier const & servicePackageInstanceId, Deactivator::ServicePackageInstanceReplicaCountEntrySPtr & servicePackageEntrySPtr)
    {
        bool entryFound = false;
        {
            AcquireReadLock readLock(entryLock_);
            if (closing_) { return ErrorCode(ErrorCodeValue::InvalidState); }        

            auto iter = servicePackageInstancesMap_.find(servicePackageInstanceId);
            if (iter != servicePackageInstancesMap_.end())
            {                
                servicePackageEntrySPtr = iter->second;
                entryFound = true;                
            }
        }

        WriteNoise(
            TraceType,
            traceId_,
            "GetServicePackageEntry: Looking for ServicePackage={0}. Entry Found={1}",                            
            servicePackageInstanceId,
            entryFound);

        return entryFound ? ErrorCode(ErrorCodeValue::Success) : ErrorCode(ErrorCodeValue::NotFound);
    }

private:    

    ApplicationIdentifier const id_;
    wstring const traceId_;
    RwLock entryLock_;

    /* These members should always be accessed under lock */
    bool closing_;  
    LONG applicationUsageCount_;
    DateTime applicationUnusedSince_;
    uint64 applicationSequenceNumber_;        

    map<ServicePackageInstanceIdentifier, Deactivator::ServicePackageInstanceReplicaCountEntrySPtr> servicePackageInstancesMap_;    
};

// ********************************************************************************************************************
// Deactivator::DeactivationEntry Implementation
//
template <class TIdentifier> 
class Deactivator::DeactivationEntry
{
    DENY_COPY(DeactivationEntry)

public:
    DeactivationEntry(TIdentifier const & id, uint64 const sequenceNumber, TimerSPtr const & timer)
        : Id(id), SequenceNumber(sequenceNumber), DeactivationTimer(timer) 
    {
    }

    virtual ~DeactivationEntry()
    {
        if (DeactivationTimer)
        {
            DeactivationTimer->Cancel();
        }
    }

public:
    TIdentifier const Id;    
    uint64 const SequenceNumber;
    TimerSPtr const DeactivationTimer;
};

template class Deactivator::DeactivationEntry<ServicePackageInstanceIdentifier>;
template class Deactivator::DeactivationEntry<ApplicationIdentifier>;

// ********************************************************************************************************************
// Deactivator::DeactivationQueue Implementation
//
template <class TIdentifier> 
class Deactivator::DeactivationQueue : Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{

    DENY_COPY(DeactivationQueue)

public:
    DeactivationQueue(wstring const & traceId)
        : map_(),
        lock_(),
        isClosed_(false),
        traceId_(traceId)
    {
    }

    virtual ~DeactivationQueue()
    {
    }

    // add deactivation entry to the queue
    ErrorCode Add(shared_ptr<Deactivator::DeactivationEntry<TIdentifier>> const & entry)
    {
        {
            AcquireWriteLock lock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }            

            auto iter = map_.find(entry->Id);
            if (iter != map_.end())
            {                
                if(iter->second->SequenceNumber < entry->SequenceNumber)
                {                    
                    WriteNoise(
                        TraceType,
                        traceId_,
                        "An entry with SequenceNumber={0} was found and removed. Add DeactivationEntry SequenceNumber={1}. A new DeactivationEntry will be added. Id={2}",
                        iter->second->SequenceNumber,
                        entry->SequenceNumber,
                        entry->Id);

                    map_.erase(iter);                          
                }
                else
                {
                    WriteNoise(
                        TraceType,
                        traceId_,
                        "An entry with SequenceNumber={0} is already present. Add DeactivationEntry SequenceNumber={1}. Id={2}",
                        iter->second->SequenceNumber,
                        entry->SequenceNumber,
                        entry->Id);

                    return ErrorCode(ErrorCodeValue::AlreadyExists);
                }
            }

            map_.insert(make_pair(entry->Id, entry));
            return ErrorCode(ErrorCodeValue::Success);            
        }
    }

    // remove the deactivation entry from the queue, if the instance id is same or lower
    ErrorCode Remove(
        TIdentifier const & id,    
        uint64 const sequenceNumber,
        __out shared_ptr<Deactivator::DeactivationEntry<TIdentifier>> & entry)
    {
        {
            AcquireWriteLock lock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

            auto iter = map_.find(id);
            if (iter != map_.end())
            {
                if(iter->second->SequenceNumber <= sequenceNumber)
                {
                    WriteNoise(
                        TraceType,
                        traceId_,
                        "An entry with SequenceNumber={0} was found and removed. Remove DeactivatorEntry SequenceNumber={1}. Id={2}",
                        iter->second->SequenceNumber,
                        sequenceNumber,
                        id);

                    entry = iter->second;
                    map_.erase(iter);                    

                    return ErrorCode(ErrorCodeValue::Success);
                }
                else
                {
                    WriteNoise(
                        TraceType,
                        traceId_,
                        "An entry with SequenceNumber={0} was found but not removed. Remove DeactivatorEntry SequenceNumber={1}. Id={2}",
                        iter->second->SequenceNumber,
                        sequenceNumber,
                        id);
                }
            }

            return ErrorCode(ErrorCodeValue::NotFound);            
        }
    }

    void Close()
    {
        {
            AcquireWriteLock lock(lock_);
            if (!isClosed_)
            {
                isClosed_ = true;
                map_.clear();
            }
        }
    }

private:

    map<TIdentifier, shared_ptr<Deactivator::DeactivationEntry<TIdentifier>>> map_;
    RwLock lock_;
    bool isClosed_;

    wstring const traceId_;
};

template class Deactivator::DeactivationQueue<ServicePackageInstanceIdentifier>;
template class Deactivator::DeactivationQueue<ApplicationIdentifier>;

// ********************************************************************************************************************
// Deactivator Implementation
//
Deactivator::Deactivator(ComponentRoot const & root, __in ApplicationManager & appManager)
    : RootedObject(root),
    appManager_(appManager),
    pendingServicePackageDeactivations_(),
    pendingApplicationDeactivations_(),
    unusedApplicationScanTimer_(),
    random_((int)SequenceNumber::GetNext())
{
    auto pendingServicePackageDeactivations = make_unique<DeactivationQueue<ServicePackageInstanceIdentifier>>(root.TraceId);
    auto pendingApplicationDeactivations = make_unique<DeactivationQueue<ApplicationIdentifier>>(root.TraceId);

    pendingServicePackageDeactivations_ = move(pendingServicePackageDeactivations);
    pendingApplicationDeactivations_ = move(pendingApplicationDeactivations);
}

Deactivator::~Deactivator()
{
}

ErrorCode Deactivator::OnOpen()
{
    auto deactivationScanInterval = HostingConfig::GetConfig().DeactivationScanInterval;
    if (deactivationScanInterval != TimeSpan::MaxValue)
    {
        auto root = Root.CreateComponentRoot();
        unusedApplicationScanTimer_ = Timer::Create("Hosting.ScanForUnusedApplications", [this, root](TimerSPtr const &) { this->ScanForUnusedApplications(); }, false);
        unusedApplicationScanTimer_->Change(deactivationScanInterval, deactivationScanInterval);
    }    

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode Deactivator::OnClose()
{
    pendingServicePackageDeactivations_->Close();
    pendingApplicationDeactivations_->Close();
    if (unusedApplicationScanTimer_)
    {
        unusedApplicationScanTimer_->Cancel();
    }

    {
        AcquireWriteLock lock(this->mapLock_);        
        this->applicationMap_.clear();
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void Deactivator::OnAbort()
{
    pendingServicePackageDeactivations_->Close();
    pendingApplicationDeactivations_->Close();
    if (unusedApplicationScanTimer_)
    {
        unusedApplicationScanTimer_->Cancel();
    }

    {
        AcquireWriteLock writeLock(this->mapLock_);
        this->applicationMap_.clear();
    }
}

/***************************** ServicePackage Deactivation ********************************/

void Deactivator::ScheduleServicePackageInstanceDeactivation(
    ServicePackageInstanceIdentifier const & servicePackageInstanceId, 
    uint64 const sequenceNumber,
    TimeSpan deactivationGraceInterval)
{    
    if(deactivationGraceInterval == TimeSpan::MaxValue)
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "ServicePackage={0}, SequenceNumber={1} is not scheduled for deactivation since DeactivationGraceInterval is TimeSpan::MaxValue.",
            servicePackageInstanceId,
            sequenceNumber);
        return;
    }
    if(HostingConfig::GetConfig().EnableProcessDebugging && appManager_.IsCodePackageLockFilePresent(servicePackageInstanceId))
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "ServicePackage={0}, SequenceNumber={1} is not scheduled for deactivation since ProcessDebugging is enabled and CodePackage lock file was found on one or more code packages.",
            servicePackageInstanceId,
            sequenceNumber);
        return;
    }
    else
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "ServicePackage={0}, SequenceNumber={1} is not used. Scheduling it for deactivation.",
            servicePackageInstanceId,
            sequenceNumber);

        auto root = this->Root.CreateComponentRoot();
        auto deactivationTimer = Timer::Create(
            "Hosting.Deactivation1",
            [root, this, servicePackageInstanceId, sequenceNumber](TimerSPtr const & timer)
        { this->OnServicePackageInstanceDeactivationCallback(timer, servicePackageInstanceId, sequenceNumber); });

        auto deactivationEntry = make_shared<ServicePackageInstanceDeactivationEntry>(servicePackageInstanceId, sequenceNumber, deactivationTimer);
        auto error = this->pendingServicePackageDeactivations_->Add(deactivationEntry);        
        if (!error.IsSuccess())
        {
            WriteTrace(
                error.IsError(ErrorCodeValue::AlreadyExists) ? LogLevel::Noise : LogLevel::Warning,
                TraceType,
                Root.TraceId,
                "Failed to schedule deactivation of ServicePackage={0}, SequenceNumber={1}, ErrorCode={2}",
                servicePackageInstanceId,
                sequenceNumber,
                error);
            return;
        }
        else
        {
            deactivationTimer->Change(deactivationGraceInterval);
        }
    }
}

void Deactivator::CancelServicePackageInstanceDeactivation(ServicePackageInstanceIdentifier const & servicePackageInstanceId, uint64 const sequenceNumber)
{
    ServicePackageInstanceDeactivationEntrySPtr entry;
    auto error = pendingServicePackageDeactivations_->Remove(servicePackageInstanceId, sequenceNumber, entry);
    if (error.IsSuccess())
    {
        entry->DeactivationTimer->Cancel();
        WriteInfo(
            TraceType,
            Root.TraceId,
            "Deactivation of ServicePackage={0} is canceled. SequenceNumber={1}",
            servicePackageInstanceId,
            sequenceNumber);
        return;
    }
}

ErrorCode Deactivator::EnsureServicePackageInstanceEntry(ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{  
    ApplicationReplicaCountEntrySPtr applicationReplicaCountEntry;
    ErrorCode error = this->GetApplicationEntry(servicePackageInstanceId.ApplicationId, applicationReplicaCountEntry);

    if(error.IsSuccess())
    {
        return applicationReplicaCountEntry->EnsureServicePackageInstanceEntry(servicePackageInstanceId);
    }

    return error;
}

ErrorCode Deactivator::IncrementUsageCount(ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{
    ApplicationReplicaCountEntrySPtr applicationReplicaCountEntry;
    ErrorCode error = this->GetApplicationEntry(servicePackageInstanceId.ApplicationId, applicationReplicaCountEntry);

    if(error.IsSuccess())
    {
        return applicationReplicaCountEntry->IncrementUsageCount(servicePackageInstanceId, *this);
    }

    return error;
}

ErrorCode Deactivator::DecrementUsageCount(ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{
    ApplicationReplicaCountEntrySPtr applicationReplicaCountEntry;
    ErrorCode error = this->GetApplicationEntry(servicePackageInstanceId.ApplicationId, applicationReplicaCountEntry);

    if(error.IsSuccess())
    {
        return applicationReplicaCountEntry->DecrementUsageCount(servicePackageInstanceId, *this);
    }

    return error;
}

ErrorCode Deactivator::ScheduleDeactivationIfNotUsed(ServicePackageInstanceIdentifier const & servicePackageInstanceId, __out bool & isDeactivationScheduled)
{
    ApplicationReplicaCountEntrySPtr applicationReplicaCountEntry;
    ErrorCode error = this->GetApplicationEntry(servicePackageInstanceId.ApplicationId, applicationReplicaCountEntry);

    if(error.IsSuccess())
    {
        return applicationReplicaCountEntry->ScheduleDeactivationIfNotUsed(servicePackageInstanceId, *this, isDeactivationScheduled);
    }

    return error;
}

bool Deactivator::OnDeactivateIfNotUsed(ServicePackageInstanceIdentifier const & servicePackageInstanceId, uint64 const sequenceNumber)
{
    ApplicationReplicaCountEntrySPtr applicationReplicaCountEntry;
    ErrorCode error = this->GetApplicationEntry(servicePackageInstanceId.ApplicationId, applicationReplicaCountEntry);

    if(error.IsSuccess())
    {
        return applicationReplicaCountEntry->OnDeactivateIfNotUsed(servicePackageInstanceId, sequenceNumber);
    }

    return false;
}

void Deactivator::OnDeactivationSuccess(ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{
    ApplicationReplicaCountEntrySPtr applicationReplicaCountEntry;
    ErrorCode error = this->GetApplicationEntry(servicePackageInstanceId.ApplicationId, applicationReplicaCountEntry);

    if(error.IsSuccess())
    {
        applicationReplicaCountEntry->OnDeactivationSuccess(servicePackageInstanceId, *this);
    }
}


void Deactivator::OnDeactivationFailed(ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{
    ApplicationReplicaCountEntrySPtr applicationReplicaCountEntry;
    ErrorCode error = this->GetApplicationEntry(servicePackageInstanceId.ApplicationId, applicationReplicaCountEntry);

    if(error.IsSuccess())
    {
        applicationReplicaCountEntry->OnDeactivationFailed(servicePackageInstanceId, *this);
    }
}

void Deactivator::OnServicePackageInstanceDeactivationCallback(
    TimerSPtr const & timer,
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    uint64 const sequenceNumber)
{
    ASSERT_IF(timer == nullptr, "OnServicePackageDeactivationCallback: Timer cannot be null");
     
    timer->Cancel(); 
    ServicePackageInstanceDeactivationEntrySPtr removed;
    this->pendingServicePackageDeactivations_->Remove(servicePackageInstanceId, sequenceNumber, removed).ReadValue();    

    WriteNoise(
        TraceType,
        Root.TraceId,
        "OnServicePackageDeactivationCallback: ServicePackage={0}, SequenceNumber={1}",
        servicePackageInstanceId,
        sequenceNumber);    

    if(this->OnDeactivateIfNotUsed(servicePackageInstanceId, sequenceNumber))
    {
        this->appManager_.DeactivateServicePackageInstance(servicePackageInstanceId);
    } 
}

bool Deactivator::IsUsed(ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{	
    ApplicationReplicaCountEntrySPtr applicationReplicaCountEntry;
    ErrorCode error = this->GetApplicationEntry(servicePackageInstanceId.ApplicationId, applicationReplicaCountEntry);

    if(error.IsSuccess())
    {
        LONG servicePackageReplicaCount;
        if(applicationReplicaCountEntry->DoesServicePackageInstanceEntryExists(servicePackageInstanceId, servicePackageReplicaCount) && servicePackageReplicaCount > 0)
        {
            return true;
        }
    }

    return false;
}


bool Deactivator::IsUsed(ApplicationIdentifier const & applicationId)
{
    ApplicationReplicaCountEntrySPtr applicationReplicaCountEntry;
    ErrorCode error = this->GetApplicationEntry(applicationId, applicationReplicaCountEntry);

    if(error.IsSuccess() && applicationReplicaCountEntry->GetUsageCount() > 0)
    {
        return true;
    }

    return false;
}

void Deactivator::GetServicePackageInstanceEntries(
    ServicePackageIdentifier const & servicePackageId,
    __out vector<ServicePackageInstanceIdentifier> & servicePackageInstances)
{
    ApplicationReplicaCountEntrySPtr applicationReplicaCountEntry;
    ErrorCode error = this->GetApplicationEntry(servicePackageId.ApplicationId, applicationReplicaCountEntry);

    if (error.IsSuccess())
    {
        applicationReplicaCountEntry->GetServicePackageInstanceEntries(servicePackageId, servicePackageInstances);
    }
}

/****************************** Application Deactivation *********************************/

ErrorCode Deactivator::EnsureApplicationEntry(ApplicationIdentifier const & applicationId)
{  
    if (this->State.Value != FabricComponentState::Opened) 
    { 
        return ErrorCode(ErrorCodeValue::InvalidState); 
    }

    {
        AcquireWriteLock writeLock(mapLock_);    

        auto iter = applicationMap_.find(applicationId);
        if (iter != applicationMap_.end())
        {
            if(iter->second->IsClosing())
            {
                return ErrorCode(ErrorCodeValue::InvalidState);
            }        
        }
        else
        {
            applicationMap_.insert(make_pair(applicationId, make_shared<ApplicationReplicaCountEntry>(applicationId, Root.TraceId)));

            WriteInfo(
                TraceType,
                Root.TraceId,
                "EnsureApplication: New entry with Application={0} inserted.",                            
                applicationId);
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void Deactivator::ScanForUnusedApplications()
{
    if (this->State.Value != FabricComponentState::Opened)
    {
        return;
    }

    DateTime lowerBound = DateTime::Now().SubtractWithMaxValueCheck(HostingConfig::GetConfig().DeactivationScanInterval);    

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Scanning for applications that are unused since {0}", 
        lowerBound);

    map<ServiceModel::ApplicationIdentifier, ApplicationReplicaCountEntrySPtr> tempApplicationMap;
    {
        AcquireReadLock readLock(mapLock_);
        tempApplicationMap = this->applicationMap_;
    }

    for(auto iter = tempApplicationMap.begin(); iter != tempApplicationMap.end(); ++iter)
    {                                    
        iter->second->ScheduleDeactivationIfNotUsedSince(lowerBound, *this);
    }
}

ErrorCode Deactivator::ScheduleDeactivationIfNotUsed(ApplicationIdentifier const & applicationId, __out bool & isDeactivationScheduled)
{
    ApplicationReplicaCountEntrySPtr applicationReplicaCountEntry;
    ErrorCode error = this->GetApplicationEntry(applicationId, applicationReplicaCountEntry);

    if(error.IsSuccess())
    {
        return applicationReplicaCountEntry->ScheduleDeactivationIfNotUsed(*this, isDeactivationScheduled);
    }    

    return error;
}

ErrorCode Deactivator::GetApplicationEntry(ServiceModel::ApplicationIdentifier const & applicationId, ApplicationReplicaCountEntrySPtr & applicationReplicaCountEntry)
{
    if (this->State.Value != FabricComponentState::Opened)
    {
        return ErrorCode(ErrorCodeValue::InvalidState);
    }

    bool entryFound = false;
    {
        AcquireReadLock readLock(mapLock_);          

        auto iter = applicationMap_.find(applicationId);
        if (iter != applicationMap_.end())
        {                
            applicationReplicaCountEntry = iter->second;
            entryFound = true;                
        }
    }
        
    if(entryFound)
    {
        WriteNoise(
            TraceType,
            Root.TraceId,
            "GetApplicationEntry: Looking for Application={0}. EntryFound={1}, UsageCount={2}",                            
            applicationId,
            entryFound,
            applicationReplicaCountEntry->GetUsageCount());

        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        WriteNoise(
            TraceType,
            Root.TraceId,
            "GetApplicationEntry: Application={0} entry is not found.",                            
            applicationId);

        return ErrorCode(ErrorCodeValue::NotFound);
    }
}

void Deactivator::ScheduleApplicationDeactivation(
    ApplicationIdentifier const & applicationId, 
    uint64 const applicationSequenceNumber)
{
    auto deactivationGraceInterval = HostingConfig::GetConfig().DeactivationGraceInterval;

    if(deactivationGraceInterval == TimeSpan::MaxValue)
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "Application={0}, SequenceNumber={1} is not scheduled for deactivation since DeactivationGraceInterval is TimeSpan::MaxValue.",
            applicationId,
            applicationSequenceNumber);
        return;
    }
    else
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "Application={0}, SequenceNumber={1} is not used. Scheduling it for deactivation.",
            applicationId,
            applicationSequenceNumber);

        auto root = this->Root.CreateComponentRoot();
        auto deactivationTimer = Timer::Create(
            "Hosting.Deactivation2",
            [root, this, applicationId, applicationSequenceNumber](TimerSPtr const & timer)
        { this->OnApplicationDeactivationCallback(timer, applicationId, applicationSequenceNumber); });

        auto deactivationEntry = make_shared<ApplicationDeactivationEntry>(applicationId, applicationSequenceNumber, deactivationTimer);
        auto error = this->pendingApplicationDeactivations_->Add(deactivationEntry);
        if (!error.IsSuccess())
        {
            WriteTrace(
                error.IsError(ErrorCodeValue::AlreadyExists) ? LogLevel::Noise : LogLevel::Warning,
                TraceType,
                Root.TraceId,
                "Failed to schedule deactivation of Application={0}, SequenceNumber={1}, ErrorCode={2}",
                applicationId,
                applicationSequenceNumber,
                error);
            return;
        }
        else
        {
            deactivationTimer->Change(deactivationGraceInterval);
        }
    }
}

void Deactivator::CancelApplicationDeactivation(ApplicationIdentifier const & applicationId, uint64 const applicationSequenceNumber)
{
    ApplicationDeactivationEntrySPtr entry;
    auto error = pendingApplicationDeactivations_->Remove(applicationId, applicationSequenceNumber, entry);
    if (error.IsSuccess())
    {
        entry->DeactivationTimer->Cancel();
        WriteInfo(
            TraceType,
            Root.TraceId,
            "Deactivation of Application={0} is canceled. SequenceNumber={1}",
            applicationId,
            applicationSequenceNumber);
        return;
    }
}

bool Deactivator::OnDeactivateIfNotUsed(ApplicationIdentifier const & applicationId, uint64 const applicationSequenceNumber)
{
    ApplicationReplicaCountEntrySPtr applicationReplicaCountEntry;
    ErrorCode error = this->GetApplicationEntry(applicationId, applicationReplicaCountEntry);

    if(error.IsSuccess())
    {
        return applicationReplicaCountEntry->OnDeactivateIfNotUsed(applicationSequenceNumber);
    }

    return false;
}

void Deactivator::OnDeactivationSuccess(ServiceModel::ApplicationIdentifier const & applicationId)
{
    if (this->State.Value != FabricComponentState::Opened)
    {
        return;
    }

    {
        AcquireWriteLock writeLock(mapLock_);    

        auto iter = applicationMap_.find(applicationId);
        if (iter != applicationMap_.end())
        {
            ASSERT_IFNOT(iter->second->IsClosing(), "Deactivated an Application without marking it for closing");

            applicationMap_.erase(iter);

            WriteNoise(
                TraceType,
                Root.TraceId,
                "OnDeactivationSuccess: Entry with Application={0} removed.",                            
                applicationId);
        }
    }
}

void Deactivator::OnApplicationDeactivationCallback(
    TimerSPtr const & timer,
    ApplicationIdentifier const & applicationId, 
    uint64 const applicationSequenceNumber)
{
    ASSERT_IF(timer == nullptr, "OnApplicationDeactivationCallback: Timer cannot be null");

    timer->Cancel(); 
    ApplicationDeactivationEntrySPtr removed;
    this->pendingApplicationDeactivations_->Remove(applicationId, applicationSequenceNumber, removed).ReadValue();

    WriteNoise(
        TraceType,
        Root.TraceId,
        "OnApplicationDeactivationCallback: Application={0}, SequenceNumber={1}",
        applicationId,
        applicationSequenceNumber);    

    if(this->OnDeactivateIfNotUsed(applicationId, applicationSequenceNumber))
    {
        this->appManager_.DeactivateApplication(applicationId);
    }    
}
