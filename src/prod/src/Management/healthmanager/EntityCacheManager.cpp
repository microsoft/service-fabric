// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;
using namespace ServiceModel;

StringLiteral const TraceComponent("EntityManager");

const LONG CacheCount = 8;

EntityCacheManager::EntityCacheManager(
    Common::ComponentRoot const & root,
    __in HealthManagerReplica & healthManagerReplica)
    : RootedObject(root)
    , healthManagerReplica_(healthManagerReplica)
    , loadingCacheCount_(CacheCount)
    , nodes_(root, healthManagerReplica)
    , partitions_(root, healthManagerReplica)
    , replicas_(root, healthManagerReplica)
    , services_(root, healthManagerReplica)
    , applications_(root, healthManagerReplica)
    , deployedApplications_(root, healthManagerReplica)
    , deployedServicePackages_(root, healthManagerReplica)
    , cluster_(root, healthManagerReplica)
    , pendingReportAmount_(0u)
    , pendingCriticalReportAmount_(0u)
    , pendingCleanupJobCount_(0u)
{
}

EntityCacheManager::~EntityCacheManager()
{
}

Common::ErrorCode EntityCacheManager::Open()
{
    // Open the caches outside the cache manager lock

    ErrorCode error(ErrorCodeValue::Success);

    // As an optimization, first enable the parent caches, then the children.
    // This will change the parent cache state first, in order for it to accept entities created by the children.

    error = cluster_.Open();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = nodes_.Open();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = applications_.Open();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = services_.Open();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = partitions_.Open();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = replicas_.Open();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = deployedApplications_.Open();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = deployedServicePackages_.Open();
    if (!error.IsSuccess())
    {
        return error;
    }

    return error;
}

Common::ErrorCode EntityCacheManager::Close()
{
    ErrorCode error(ErrorCodeValue::Success);
    vector<ReportRequestContext> blockedReports;
    vector<SequenceStreamRequestContext> blockedStreams;

    { // lock
        AcquireWriteLock grab(lock_);
        swap(blockedReports, blockedReports_);
        swap(blockedStreams, blockedStreams_);

        // Set cache count to high number, so even if the loading of the last cache completes, it won't have any effect
        loadingCacheCount_ = numeric_limits<LONG>::max();
    } // endlock

    //
    // Close the caches outside the cache manager lock, in reverse order to Enable
    //
    ErrorCode innerError;

    innerError = deployedServicePackages_.Close();
    if (!innerError.IsSuccess())
    {
        error.Overwrite(innerError);
    }

    innerError = deployedApplications_.Close();
    if (!innerError.IsSuccess())
    {
        error.Overwrite(innerError);
    }

    innerError = replicas_.Close();
    if (!innerError.IsSuccess())
    {
        error.Overwrite(innerError);
    }

    innerError = partitions_.Close();
    if (!innerError.IsSuccess())
    {
        error.Overwrite(innerError);
    }

    innerError = services_.Close();
    if (!innerError.IsSuccess())
    {
        error.Overwrite(innerError);
    }

    innerError = applications_.Close();
    if (!innerError.IsSuccess())
    {
        error.Overwrite(innerError);
    }

    innerError = nodes_.Close();
    if (!innerError.IsSuccess())
    {
        error.Overwrite(innerError);
    }

    innerError = cluster_.Close();
    if (!innerError.IsSuccess())
    {
        error.Overwrite(innerError);
    }

    //
    // Complete block requests outside the cache manager lock
    //
    for (auto it = blockedReports.begin(); it != blockedReports.end(); ++it)
    {
        healthManagerReplica_.QueueReportForCompletion(move(*it), ErrorCode(ErrorCodeValue::ObjectClosed));
    }

    for (auto it = blockedStreams.begin(); it != blockedStreams.end(); ++it)
    {
        healthManagerReplica_.QueueSequenceStreamForCompletion(move(*it), ErrorCode(ErrorCodeValue::ObjectClosed));
    }

    return error;
}

void EntityCacheManager::OnCacheLoadCompleted()
{
    bool allCachesLoadCompleted = false;

    vector<ReportRequestContext> blockedReports;
    vector<SequenceStreamRequestContext> blockedStreams;

    { // lock
        AcquireWriteLock lock(lock_);
        --loadingCacheCount_;

        if (loadingCacheCount_ == 0)
        {
            allCachesLoadCompleted = true;
            blockedReports.swap(blockedReports_);
            blockedStreams.swap(blockedStreams_);
        }
    } // endlock

    if (allCachesLoadCompleted)
    {
        healthManagerReplica_.OnCacheLoadCompleted();

        healthManagerReplica_.WriteInfo(
            TraceComponent,
            "All caches loaded, processing blocked requests: {0} reports, {1} sequence streams",
            blockedReports.size(),
            blockedStreams.size());

        // The requests have already been counted, so process them directly
        ProcessReports(move(blockedReports));

        for (auto it = blockedStreams.begin(); it != blockedStreams.end(); ++it)
        {
            ProcessSequenceStream(move(*it));
        }
    }
}

bool EntityCacheManager::IsReady() const
{
    // Return whether all caches are loaded.
    return (loadingCacheCount_ == 0);
}

void EntityCacheManager::AddReports(vector<ReportRequestContext> && contexts)
{
    if (contexts.empty())
    {
        return;
    }

    vector<ReportRequestContext> acceptedReports;
    vector<ReportRequestContext> rejectedReports;
    TryAccept(move(contexts), acceptedReports, rejectedReports);

    if (!IsReady())
    { // lock
        AcquireWriteLock grab(lock_);
        for (auto it = acceptedReports.begin(); it != acceptedReports.end(); ++it)
        {
            HMEvents::Trace->BlockReport(it->Report.EntityInformation->EntityId, *it);
            blockedReports_.push_back(move(*it));
        }
    } // endlock
    else
    {
        ProcessReports(move(acceptedReports));
    }

    // Reject the reports that couldn't be accepted
    for (auto it = rejectedReports.begin(); it != rejectedReports.end(); ++it)
    {
        healthManagerReplica_.QueueReportForCompletion(move(*it), ErrorCode(ErrorCodeValue::ServiceTooBusy));
    }
}

void EntityCacheManager::ProcessReports(vector<ReportRequestContext> && contexts)
{
    // For each request, find the entity and enqueue the request under the cache lock
    // if the request can be accepted.
    // The actual processing will be done when it's scheduled by the job queue manager
    // and it will be preceded by validation.
    // Note that if an entity doesn't exist, it will be created:
    //    - If this is the first report, the request will be accepted (no validation to check against).
    //    - If the replica just became primary and doesn't have information that is kept in the database,
    //        it's ok to create the entity and modify it accordingly when the actual entry is processed.
    for (auto it = contexts.begin(); it != contexts.end(); ++it)
    {
        switch (it->Kind)
        {
        case HealthEntityKind::Replica:
            replicas_.AddReport(move(*it));
            break;
        case HealthEntityKind::Partition:
            partitions_.AddReport(move(*it));
            break;
        case HealthEntityKind::Node:
            nodes_.AddReport(move(*it));
            break;
        case HealthEntityKind::DeployedServicePackage:
            deployedServicePackages_.AddReport(move(*it));
            break;
        case HealthEntityKind::DeployedApplication:
            deployedApplications_.AddReport(move(*it));
            break;
        case HealthEntityKind::Application:
            applications_.AddReport(move(*it));
            break;
        case HealthEntityKind::Service:
            services_.AddReport(move(*it));
            break;
        case HealthEntityKind::Cluster:
            cluster_.AddReport(move(*it));
            break;
        default:
            TRACE_LEVEL_AND_TESTASSERT(healthManagerReplica_.WriteInfo, TraceComponent, "{0}: Invalid context kind", *it);
            // Complete the report with non-retryable error
            healthManagerReplica_.QueueReportForCompletion(move(*it), ErrorCode(ErrorCodeValue::InvalidArgument));
            break;
        }
    }
}

void EntityCacheManager::AddSequenceStreamContext(
    SequenceStreamRequestContext && context)
{
    if (TryAccept(context))
    {
        if (!IsReady())
        {
            // Cache the context for later processing
            HMEvents::Trace->BlockSS(context);
            AcquireWriteLock grab(lock_);
            blockedStreams_.push_back(move(context));
        }
        else
        {
            ProcessSequenceStream(move(context));
        }
    }
    else
    {
        healthManagerReplica_.QueueSequenceStreamForCompletion(move(context), ErrorCode(ErrorCodeValue::ServiceTooBusy));
    }
}

void EntityCacheManager::ProcessSequenceStream(
    SequenceStreamRequestContext && context)
{
    switch (context.EntityKind)
    {
    case HealthEntityKind::Replica:
        replicas_.AddSequenceStream(move(context));
        break;
    case HealthEntityKind::Partition:
        partitions_.AddSequenceStream(move(context));
        break;
    case HealthEntityKind::Node:
        nodes_.AddSequenceStream(move(context));
        break;
    case HealthEntityKind::Service:
        services_.AddSequenceStream(move(context));
        break;
    default:
        TRACE_LEVEL_AND_TESTASSERT(healthManagerReplica_.WriteInfo, TraceComponent, "{0}: Invalid kind", context);

        healthManagerReplica_.QueueSequenceStreamForCompletion(move(context), ErrorCode(ErrorCodeValue::InvalidArgument));
        break;
    }
}

Common::ErrorCode EntityCacheManager::GetSequenceStreamProgress(
    SequenceStreamId const & sequenceStreamId,
    FABRIC_INSTANCE_ID instance,
    bool getHighestLsn,
    __inout SequenceStreamResult & sequenceStreamResult)
{
    if (!IsReady())
    {
        healthManagerReplica_.WriteWarning(TraceComponent,
            "Reject GetSequenceStreamProgress request for {0} as cache is not ready",
            sequenceStreamId);
        return ErrorCodeValue::ServiceTooBusy;
    }

    ErrorCode error(ErrorCodeValue::Success);
    switch(HealthEntityKind::FromHealthInformationKind(sequenceStreamId.Kind))
    {
        case HealthEntityKind::Replica:
            error = replicas_.GetSequenceStreamProgress(sequenceStreamId.SourceId, instance, getHighestLsn, /*out*/sequenceStreamResult);
            break;
        case HealthEntityKind::Partition:
            error = partitions_.GetSequenceStreamProgress(sequenceStreamId.SourceId, instance, getHighestLsn, /*out*/sequenceStreamResult);
            break;
        case HealthEntityKind::Node:
            error = nodes_.GetSequenceStreamProgress(sequenceStreamId.SourceId, instance, getHighestLsn, /*out*/sequenceStreamResult);
            break;
        case HealthEntityKind::Service:
            error = services_.GetSequenceStreamProgress(sequenceStreamId.SourceId, instance, getHighestLsn, /*out*/sequenceStreamResult);
            break;
        default:
            TRACE_LEVEL_AND_TESTASSERT(healthManagerReplica_.WriteInfo, TraceComponent, "Get SS: Invalid kind {0}", sequenceStreamId.Kind);

            error = ErrorCode(ErrorCodeValue::InvalidArgument);
            break;
    }

    return error;
}

Common::ErrorCode EntityCacheManager::AcceptSequenceStream(
    ActivityId const & activityId,
    SequenceStreamId const & sequenceStreamId,
    FABRIC_INSTANCE_ID instance,
    FABRIC_SEQUENCE_NUMBER upToLsn)
{
    if (!IsReady())
    {
        healthManagerReplica_.WriteWarning(
            TraceComponent,
            "{0}: Reject AcceptSequenceStream request for {1} as cache is not ready",
            activityId,
            sequenceStreamId);
        return ErrorCodeValue::ServiceTooBusy;
    }

    ErrorCode error(ErrorCodeValue::Success);
    switch(HealthEntityKind::FromHealthInformationKind(sequenceStreamId.Kind))
    {
        case HealthEntityKind::Partition:
            error = partitions_.AcceptSequenceStream(activityId, sequenceStreamId.SourceId, instance, upToLsn);
            break;
        case HealthEntityKind::Node:
            error = nodes_.AcceptSequenceStream(activityId, sequenceStreamId.SourceId, instance, upToLsn);
            break;
        case HealthEntityKind::Service:
            error = services_.AcceptSequenceStream(activityId, sequenceStreamId.SourceId, instance, upToLsn);
            break;
        default:
            TRACE_LEVEL_AND_TESTASSERT(healthManagerReplica_.WriteInfo, TraceComponent, " {0}: Invalid kind {1}", activityId, sequenceStreamId.Kind);

            error = ErrorCode(ErrorCodeValue::InvalidArgument);
            break;
    }

    return error;
}

void EntityCacheManager::TryAccept(
    std::vector<ReportRequestContext> && contexts,
    __in std::vector<ReportRequestContext> & acceptedReports,
    __in std::vector<ReportRequestContext> & rejectedReports)
{
    for (auto && context : contexts)
    {
        if (TryAccept(context))
        {
            acceptedReports.push_back(move(context));
        }
        else
        {
            rejectedReports.push_back(move(context));
        }
    }

    if (rejectedReports.empty())
    {
        HMEvents::Trace->CacheQueuedReports(
            acceptedReports[0].ReplicaActivityId.TraceId,
            acceptedReports.size(),
            HealthManagerCounters->NumberOfQueuedReports.Value,
            HealthManagerCounters->NumberOfQueuedCriticalReports.Value,
            HealthManagerCounters->SizeOfQueuedReports.Value,
            HealthManagerCounters->SizeOfQueuedCriticalReports.Value);
    }
    else
    {
        HMEvents::Trace->QueueReports(
            rejectedReports[0].ReplicaActivityId.TraceId,
            acceptedReports.size(),
            rejectedReports.size(),
            HealthManagerCounters->NumberOfQueuedReports.Value,
            HealthManagerCounters->NumberOfQueuedCriticalReports.Value,
            HealthManagerCounters->SizeOfQueuedReports.Value,
            HealthManagerCounters->SizeOfQueuedCriticalReports.Value);
    }
}

bool EntityCacheManager::TryAccept(RequestContext const & context)
{
    uint64 contextSize = context.GetEstimatedSize();
    uint64 max;
    uint64 count;
    if (ManagementConfig::GetConfig().EnableMaxPendingHealthReportSizeEstimation)
    {
        // Use size estimation to determine whether the request can be accepted
        max = static_cast<uint64>(ManagementConfig::GetConfig().MaxPendingHealthReportSizeBytes);
        if (context.Priority == Priority::Critical)
        {
            count = (pendingCriticalReportAmount_ += contextSize) + contextSize;
        }
        else
        {
            ASSERT_IF(context.Priority == Priority::NotAssigned, "TryAccept(context): Priority should be assigned for {0}", context.ReplicaActivityId.TraceId);
            count = (pendingReportAmount_ += contextSize) + contextSize;
        }
    }
    else
    {
        // Use count estimation to determine whether the context can be accepted
        max = static_cast<uint64>(ManagementConfig::GetConfig().MaxPendingHealthReportCount);
        if (context.Priority == Priority::Critical)
        {
            count = ++pendingCriticalReportAmount_;
        }
        else
        {
            ASSERT_IF(context.Priority == Priority::NotAssigned, "TryAccept(context): Priority should be assigned for {0}", context.ReplicaActivityId.TraceId);
            count = ++pendingReportAmount_;
        }
    }

    bool isAccepted = (count <= max);

    // First set context accept for processing result, then update perf counters, which use this information
    context.IsAcceptedForProcessing = isAccepted;
    this->HealthManagerCounters->OnRequestContextTryAcceptCompleted(context);

    return isAccepted;
}

void EntityCacheManager::OnContextCompleted(RequestContext const & context)
{
    uint64 contextSize = context.GetEstimatedSize();

    if (ManagementConfig::GetConfig().EnableMaxPendingHealthReportSizeEstimation)
    {
        if (context.Priority == Priority::Critical)
        {
            pendingCriticalReportAmount_ -= contextSize;
        }
        else
        {
            pendingReportAmount_ -= contextSize;
        }

        healthManagerReplica_.WriteInfo(
            TraceComponent,
            "OnContextCompleted: pending={0} critical={1}",
            pendingReportAmount_.load(),
            pendingCriticalReportAmount_.load());
    }
    else
    {
        if (context.Priority == Priority::Critical)
        {
            --pendingCriticalReportAmount_;
        }
        else
        {
            --pendingReportAmount_;
        }
    }

    this->HealthManagerCounters->OnRequestContextCompleted(context);
}

Common::ErrorCode EntityCacheManager::TryCreateCleanupJob(Common::ActivityId const & activityId, std::wstring const & entityId)
{
    // Tentatively increment the pending cleanup job count,
    // to optimize for the path where there are not too many job items.
    uint64 count = pendingCleanupJobCount_++;
    if (count >= static_cast<uint64>(ManagementConfig::GetConfig().MaxPendingHealthCleanupJobCount))
    {
        // Revert the count to previous value
        --pendingCleanupJobCount_;
        HMEvents::Trace->CreateCleanupJobFailed(
            entityId,
            activityId,
            count,
            ManagementConfig::GetConfig().MaxPendingHealthCleanupJobCount);
        return ErrorCode(ErrorCodeValue::ServiceTooBusy);
    }

    return ErrorCode::Success();
}

void EntityCacheManager::OnCleanupJobCompleted()
{
    --pendingCleanupJobCount_;
}
