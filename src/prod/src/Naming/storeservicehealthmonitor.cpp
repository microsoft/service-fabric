// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Reliability;
using namespace ServiceModel;
using namespace Naming;

StringLiteral const TraceComponent("HealthMonitor");

TimeSpan const HealthReportSendOverhead = TimeSpan::FromSeconds(30);

// --------------------------------------------------
// HealthMonitoredOperationName
// --------------------------------------------------
namespace Naming
{
    namespace HealthMonitoredOperationName
    {
        void WriteToTextWriter(TextWriter & w, Enum e)
        {
            switch (e)
            {
            case PrimaryRecoveryStarted: w << "PrimaryRecoveryStarted"; break;
            case AOCreateName: w << "AOCreateName"; break;
            case AODeleteName: w << "AODeleteName"; break;
            case AOCreateService: w << "AOCreateService"; break;
            case AODeleteService: w << "AODeleteService"; break;
            case AOUpdateService: w << "AOUpdateService"; break;
            case NOCreateName: w << "NOCreateName"; break;
            case NODeleteName: w << "NODeleteName"; break;
            case NOCreateService: w << "NOCreateService"; break;
            case NODeleteService: w << "NODeleteService"; break;
            case NOUpdateService: w << "NOUpdateService"; break;
            case NameExists: w << "NameExists"; break;
            case EnumerateProperties: w << "EnumerateProperties"; break;
            case EnumerateNames: w << "EnumerateNames"; break;
            case GetServiceDescription: w << "GetServiceDescription"; break;
            case PrefixResolve: w << "PrefixResolve"; break;
            case PropertyBatch: w << "PropertyBatch"; break;
            case InvalidOperation: w << "InvalidOperation"; break;
            case PrimaryRecovery: w << "PrimaryRecovery"; break;
            default: Assert::CodingError("Invalid state for internal enum: {0}", static_cast<int>(e));
            };
        }

        ENUM_STRUCTURED_TRACE(HealthMonitoredOperationName, PrimaryRecoveryStarted, LastValidEnum);
    }
}

// --------------------------------------------------
// HealthMonitoredOperation
// --------------------------------------------------
HealthMonitoredOperation::HealthMonitoredOperation(
    Common::ActivityId const & activityId,
    HealthMonitoredOperationName::Enum operationName,
    std::wstring const & operationMetadata,
    Common::TimeSpan const & maxAllowedDuration)
    : activityId_(activityId)
    , operationName_(operationName)
    , operationMetadata_(operationMetadata)
    , maxAllowedDuration_(maxAllowedDuration)
    , startTime_(Stopwatch::Now())
    , completedTime_(StopwatchTime::MaxValue)
    , completedError_(ErrorCodeValue::Success)
    , lastReportTime_(StopwatchTime::MaxValue)
    , lastReportTTL_(TimeSpan::MaxValue)
{
}

HealthMonitoredOperation::~HealthMonitoredOperation()
{
}

bool HealthMonitoredOperation::IsSlowDetected() const
{
    return lastReportTime_ != StopwatchTime::MaxValue;
}

bool HealthMonitoredOperation::MarkSendHealthReport(Common::StopwatchTime const & now)
{
    if (IsCompleted())
    {
        // No report is sent for completed operations
        return false;
    }

    if (IsSlowDetected())
    {
        // Check whether the last report time was sent too long ago
        bool resend;
        if (lastReportTTL_ <= HealthReportSendOverhead)
        {
            resend = true;
        }
        else
        {
            TimeSpan resendInterval = lastReportTTL_ - HealthReportSendOverhead;
            resend = (now - lastReportTime_ >= resendInterval);
        }
        
        if (resend)
        {
            lastReportTime_ = now;
            return true;
        }

        return false;
    }

    // The operation wasn't previously too slow, check if it still respects duration
    if (now - startTime_ >= maxAllowedDuration_)
    {
        lastReportTime_ = now;
        return true;
    }

    return false;
}

std::wstring HealthMonitoredOperation::GenerateHealthReportExtraDescription() const
{
    if (IsCompleted())
    {
        return wformatString(HMResource::GetResources().NamingOperationSlowCompleted, operationName_, startTime_.ToDateTime(), completedError_, maxAllowedDuration_);
    }
    else
    {
        ASSERT_IFNOT(completedError_.IsSuccess(), "{0}: {1}+{2}: completed error set even if operation is not yet completed", activityId_, operationName_, operationMetadata_);
        return wformatString(HMResource::GetResources().NamingOperationSlow, operationName_, startTime_.ToDateTime(), maxAllowedDuration_);
    }
}

std::wstring HealthMonitoredOperation::GenerateHealthReportProperty() const
{
    if (operationMetadata_.empty())
    {
        return wformatString("{0}", operationName_);
    }

    return wformatString("{0}.{1}", operationName_, operationMetadata_);
}

HealthReport HealthMonitoredOperation::GenerateHealthReport(
    ServiceModel::EntityHealthInformationSPtr && entityInfo,
    Common::TimeSpan const & ttl,
    int64 reportSequenceNumber) const
{
    auto dynamicProperty = this->GenerateHealthReportProperty();
    auto extraDescription = this->GenerateHealthReportExtraDescription();
    
    // Remember the ttl to know when to send next report
    lastReportTTL_ = ttl;

    auto report = HealthReport::CreateSystemHealthReport(
        IsCompleted() ? SystemHealthReportCode::Naming_OperationSlowCompleted : SystemHealthReportCode::Naming_OperationSlow,
        move(entityInfo),
        dynamicProperty,
        extraDescription,
        reportSequenceNumber,
        ttl,
        AttributeList());
    StoreServiceHealthMonitor::WriteInfo(TraceComponent, "{0}: Created {1}", activityId_, report);
    return move(report);
}

void HealthMonitoredOperation::ResetCompletedInfo(Common::ActivityId const & activityId)
{
    if (IsCompleted())
    {
        // A new retry is started for the same operation.
        // Mark slow detected false, so a new report will be sent for the new retry.
        completedTime_ = StopwatchTime::MaxValue;
        lastReportTime_ = StopwatchTime::MaxValue;
        lastReportTTL_ = TimeSpan::MaxValue;
        
        // Ignore previous completed value
        completedError_.ReadValue();
        completedError_ = ErrorCode(ErrorCodeValue::Success);

        if (activityId != activityId_)
        {
            StoreServiceHealthMonitor::WriteInfo(
                TraceComponent,
                "{0}: {1}+{2}: ResetCompleted on new request {3}",
                activityId_,
                operationName_,
                operationMetadata_,
                activityId);
            // Set the activity id to the new activity id
            activityId_ = activityId;
        }
        else
        {
            StoreServiceHealthMonitor::WriteInfo(
                TraceComponent,
                "{0}: {1}+{2}: ResetCompleted on retry",
                activityId_,
                operationName_,
                operationMetadata_);
        }
    }
}

bool HealthMonitoredOperation::ShouldBeRemoved(Common::StopwatchTime const & now) const
{
    return IsCompleted() && (completedTime_ + NamingConfig::GetConfig().NamingServiceFailedOperationHealthGraceInterval <= now);
}

bool HealthMonitoredOperation::IsCompleted() const 
{ 
    return completedTime_ != StopwatchTime::MaxValue; 
}

void HealthMonitoredOperation::Complete(Common::ErrorCode const & error)
{
    completedTime_ = Stopwatch::Now();
    completedError_.ReadValue();
    completedError_ = error;
}

// --------------------------------------------------
// StoreServiceHealthMonitor
// --------------------------------------------------
StoreServiceHealthMonitor::StoreServiceHealthMonitor(
    ComponentRoot const & root,
    Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    __in FederationWrapper & federation)
    : RootedObject(root)
    , Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::NamingStoreService>(partitionId, replicaId)
    , federation_(federation)
    , healthClient_()
    , lock_()
    , timer_()
    , operations_()
    , isOpen_(false)
{ 
    WriteNoise(TraceComponent, "{0}: ctor {1}", this->TraceId, TextTraceThis);
}

StoreServiceHealthMonitor::~StoreServiceHealthMonitor()
{
    WriteNoise(TraceComponent, "{0}: ~dtor {1}", this->TraceId, TextTraceThis);
}

bool StoreServiceHealthMonitor::Comparator::operator()(HealthMonitoredOperationKey const & left, HealthMonitoredOperationKey const & right) const
{
    // Same operation name, check the metadata
    if (left.first == right.first)
    {
        return left.second < right.second;
    }

    // Check operation name
    return left.first < right.first;
}

void StoreServiceHealthMonitor::Open()
{
    AcquireWriteLock lock(lock_);
    ASSERT_IF(isOpen_, "Invalid state for Open: StoreServiceHealthMonitor {0} is already opened", static_cast<void*>(this));

    isOpen_ = true;

    if (!healthClient_)
    {
        healthClient_ = federation_.Federation.GetHealthClient();
    }

    // Create the scan timer, but do not enable it yet, since there are no operations
    if (!timer_)
    {
        auto root = this->Root.CreateAsyncOperationRoot();
        timer_ = Timer::Create(TimerTagDefault, [this, root](Common::TimerSPtr const &) { this->OnTimerCallback(); });
    }

    DisableTimerCallerHoldsLock();
}

void StoreServiceHealthMonitor::Close()
{
    AcquireWriteLock grab(lock_);
    if (!isOpen_)
    {
        return;
    }

    isOpen_ = false;

    if (timer_)
    {
        timer_->Cancel();
        timer_.reset();
    }

    // Since the instance of the primary replica is closed, HM removes all reports on it
    // Clear the operations so the 2 views match.
    operations_.clear();
}

void StoreServiceHealthMonitor::AddMonitoredOperation(
    Common::ActivityId const & activityId,
    HealthMonitoredOperationName::Enum operationName,
    std::wstring const & operationMetadata,
    Common::TimeSpan const & maxAllowedDuration)
{
    AcquireWriteLock lock(lock_);
    if (!isOpen_)
    {
        WriteNoise(TraceComponent, "{0}: skip add monitored operation as the health monitor is not opened", activityId);
        return;
    }

    // Create key with a reference to the input operationMetadata
    auto itFind = operations_.find(HealthMonitoredOperationKey(operationName, operationMetadata));
    if (itFind != operations_.end())
    {
        // There is already an entry, this can be a client retry for an already existing operation.
        // Eg. user called CreateService, timed out on client, retries. 
        // Naming service has received the previous request and it's processing it.
        // Do not add, keep the old operation.
        // If the operation was previously completed with error, reset the completed state.
        itFind->second->ResetCompletedInfo(activityId);
        return;
    }
    
    auto operation = make_shared<HealthMonitoredOperation>(
        activityId,
        operationName,
        operationMetadata,
        maxAllowedDuration);
    // Create a new key to insert in map with reference to the operation metadata inside the operation
    HealthMonitoredOperationKey mapKey(operationName, operation->OperationMetadata);

#ifdef DBG
    auto insertResult = operations_.insert(make_pair(mapKey, move(operation))); 
    ASSERT_IFNOT(insertResult.second, "{0}: {1}+{2}: the operation is already in the monitored list", activityId, operationName, operationMetadata);
#else
    operations_.insert(make_pair(mapKey, move(operation)));
#endif

    // If this is the first item added, start the timer
    if (operations_.size() == 1)
    {
        EnableTimerCallerHoldsLock();
    }
}

void StoreServiceHealthMonitor::CompleteSuccessfulMonitoredOperation(
    Common::ActivityId const & activityId,
    HealthMonitoredOperationName::Enum operationName,
    std::wstring const & operationMetadata)
{
    CompleteMonitoredOperation(activityId, operationName, operationMetadata, ErrorCode::Success(), false);
}

void StoreServiceHealthMonitor::CompleteMonitoredOperation(
    Common::ActivityId const & activityId,
    HealthMonitoredOperationName::Enum operationName,
    std::wstring const & operationMetadata,
    Common::ErrorCode const & error,
    bool keepOperation)
{
    HealthMonitoredOperationSPtr deletedOp;
    FABRIC_SEQUENCE_NUMBER reportSequenceNumber = FABRIC_INVALID_SEQUENCE_NUMBER;

    { // lock
        AcquireWriteLock lock(lock_);
        if (!isOpen_)
        {
            // Nothing to do in this case, when replica is removed from health store all reports are cleaned up
            return;
        }
        
        // Get the operation, if it exists, so we can clear reports on it
        auto itOp = operations_.find(HealthMonitoredOperationKey(operationName, operationMetadata));
        if (itOp == operations_.end())
        {
            WriteNoise(TraceComponent, "{0}: {1}+{2}: Monitored operation was not found", activityId, operationName, operationMetadata);
            return;
        }

        // Check that the operation is for the same activity id. Keep only one entry,
        // the first one since we care about start time.
        if (itOp->second->ActivityId != activityId)
        {
            WriteInfo(TraceComponent, "{0}: Complete {1}+{2}: Monitored operation has different activityId {3}, skip.", activityId, operationName, operationMetadata, itOp->second->ActivityId);
            return;
        }

        itOp->second->Complete(error);
        if (keepOperation)
        {
            // The operation didn't yet complete successfully.
            // It's possible there may be more retries from either authority owner or user. 
            // Keep track of initial startup time by keeping the old operation for a while.
            deletedOp = itOp->second;
        }
        else
        {
            // Remove the operation from the map under the lock
            deletedOp = move(itOp->second);
            operations_.erase(itOp);

            // If there are no more operations, stop the timer
            if (operations_.empty())
            {
                DisableTimerCallerHoldsLock();
            }
        }

        // Generate the health report sequence number under the lock
        if (deletedOp->IsSlowDetected())
        {
            reportSequenceNumber = SequenceNumber::GetNext();
        }
    } // endlock

    // Send report to clear the previous reports (if any), outside the lock
    if (reportSequenceNumber != FABRIC_INVALID_SEQUENCE_NUMBER)
    {
        // The report stays in store for ttl before being removed, so users can have a chance to see it
        // If the operation is kept for longer, use the configured TTL.
        // Otherwise, use a small TTL so the operation is cleaned up quickly.
        auto ttl = keepOperation ?
            NamingConfig::GetConfig().NamingServiceFailedOperationHealthReportTimeToLive :
            TimeSpan::FromSeconds(1);

        auto healthReport = deletedOp->GenerateHealthReport(
            CreateNamingPrimaryEntityInfo(),
            ttl,
            reportSequenceNumber);
        
        // Once created, the health client is not changed, so it's OK to access outside lock
        auto reportError = healthClient_->AddHealthReport(move(healthReport));
        if (!reportError.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: Add completed report returned {1}",
                deletedOp->ActivityId,
                reportError);
        }
    }
}

void StoreServiceHealthMonitor::OnTimerCallback()
{
    // Look for all operations that took longer than expected and report health on them
    using MonitoredOperationWithSequence = pair<int64, HealthMonitoredOperationSPtr>;
    vector<MonitoredOperationWithSequence> slowOperations;

    StopwatchTime now = Stopwatch::Now();

    int count = 0;
    int maxCount = NamingConfig::GetConfig().MaxNamingServiceHealthReports;
    if (maxCount == 0)
    {
        maxCount = numeric_limits<int>::max();
    }
    
    { // lock
        AcquireWriteLock lock(lock_);
        if (!isOpen_ || !healthClient_)
        {
            return;
        }

        OperationMap tempOperations;
        for (auto && op : operations_)
        {
            if (op.second->ShouldBeRemoved(now))
            {
                // Remove operations that have been completed but not retried in a more than grace period
                continue;
            }

            if (count < maxCount && op.second->MarkSendHealthReport(now))
            {
                // Generate a sequence number under the lock to prevent races when adding to health client
                slowOperations.push_back(make_pair(SequenceNumber::GetNext(), op.second));
                ++count;
            }

            tempOperations.insert(move(op));
        }

        operations_ = move(tempOperations);

        // If there are no more operations, stop the timer
        if (operations_.empty())
        {
            DisableTimerCallerHoldsLock();
        }
    } // endlock

    if (count >= maxCount)
    {
        WriteInfo(TraceComponent, "TimerCallback: there are more slow operations that max {0}, report {0}", maxCount);
    }

    if (!slowOperations.empty())
    {
        vector<HealthReport> reports;
        for (auto & op : slowOperations)
        {
            auto healthReport = op.second->GenerateHealthReport(
                CreateNamingPrimaryEntityInfo(),
                NamingConfig::GetConfig().NamingServiceSlowOperationHealthReportTimeToLive,
                op.first/*reportSequenceNumber*/);
            reports.push_back(move(healthReport));
        }

        // Once created, the health client is not changed, so it's ok to access outside lock
        auto error = healthClient_->AddHealthReports(move(reports));
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: Adding reports for {1} operations returned error {2}",
                slowOperations[0].second->ActivityId,
                slowOperations.size(),
                error);
        }
    }
}

void StoreServiceHealthMonitor::DisableTimerCallerHoldsLock()
{
    timer_->Change(TimeSpan::MaxValue);
}

void StoreServiceHealthMonitor::EnableTimerCallerHoldsLock()
{
    auto scanTime = NamingConfig::GetConfig().NamingServiceHealthReportingTimerInterval;
    timer_->Change(scanTime, scanTime);
}

ServiceModel::EntityHealthInformationSPtr StoreServiceHealthMonitor::CreateNamingPrimaryEntityInfo() const
{
    // The naming service replica doesn't have access to the instance id.
    // Because of this, it can't report on transitions only, because reports may be delayed
    // and applied on the new instance. In this case, they will not be cleaned up.
    // Instead, we report periodically with RemoveWhenExpired=true,
    // so even if reports are applied on a new instance, they will be cleaned up after TTL.
    return EntityHealthInformation::CreateStatefulReplicaEntityHealthInformation(
        this->PartitionId,
        this->ReplicaId,
        FABRIC_INVALID_INSTANCE_ID);
}
