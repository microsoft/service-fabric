// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace ServiceModel;
using namespace Transport;
using namespace Query;
using namespace Store;
using namespace SystemServices;
using namespace Management;
using namespace Management::HealthManager;
using namespace ServiceModel;
using namespace std;
using namespace ClientServerTransport;

StringLiteral const TraceComponent("HMReplica");
StringLiteral const TraceUpgrade("Upgrade");

// **********************************
// Internal class - JobQueueHolder
// **********************************
class HealthManagerReplica::JobQueueHolder
{
    DENY_COPY(JobQueueHolder)
public:
    explicit JobQueueHolder(__in HealthManagerReplica & replica)
        : replica_(replica)
        , queryJobQueue_(
            L"HMQuery",
            replica,
            ManagementConfig::GetConfig().HealthProcessingQueueThreadCount,
            nullptr,
            ManagementConfig::GetConfig().MaxPendingHealthQueryCount)
        , completeRequestJobQueue_(
            L"HMCompleteRequest",
            replica,
            ManagementConfig::GetConfig().HealthProcessingQueueThreadCount,
            nullptr,
            UINT64_MAX) /*the items must be completed or else the async operations are leaked*/
    {
        // Throttle the query job queue until all items are loaded in memory,
        // at which point the query job queue will be resumed
        queryJobQueue_.SetThrottle(true);

        // Set extra tracing for the job queue if needed, which traces the thread ids used for processing.
        if (ManagementConfig::GetConfig().EnableQueryJobQueueTraceProcessingThreads)
        {
            queryJobQueue_.SetTraceProcessingThreads(true);
        }
    }

    void OnCacheLoadCompleted()
    {
        queryJobQueue_.Resume();
    }

    Common::ErrorCode AddQuery(
        Common::AsyncOperationSPtr const & queryOperation,
        __in IQueryProcessor * queryProcessor)
    {
        QueryJobItem jobItem(queryOperation, queryProcessor);
        if (!queryJobQueue_.Enqueue(move(jobItem)))
        {
            return ErrorCode(
                ErrorCodeValue::ServiceTooBusy,
                wformatString("{0} {1}", Resources::GetResources().QueryJobQueueFull, queryJobQueue_.GetQueueLength()));
        }

        return ErrorCode::Success();
    }

    void QueueReportForCompletion(ReportRequestContext && context, Common::ErrorCode && error)
    {
        CompleteRequestJobItem jobItem(make_unique<ReportRequestContext>(move(context)), move(error));
        if (!completeRequestJobQueue_.Enqueue(move(jobItem)))
        {
            // Enqueue can fail if the job queue was closed
            replica_.WriteNoise(TraceComponent, "{0}: Adding report to completed request job queue failed", replica_.PartitionedReplicaId.TraceId);
        }
    }

    void QueueSequenceStreamForCompletion(SequenceStreamRequestContext && context, Common::ErrorCode && error)
    {
        CompleteRequestJobItem jobItem(make_unique<SequenceStreamRequestContext>(move(context)), move(error));
        if (!completeRequestJobQueue_.Enqueue(move(jobItem)))
        {
            // Enqueue can fail if the job queue was closed
            replica_.WriteNoise(TraceComponent, "{0}: Adding sequence stream to completed request job queue failed", replica_.PartitionedReplicaId.TraceId);
        }
    }

    void Close()
    {
        // Close the job queues to release the root.
        queryJobQueue_.Close();
        completeRequestJobQueue_.Close();

        auto maxCloseWaitDuration = ManagementConfig::GetConfig().MaxCloseJobQueueWaitDuration;

        // Wait for all items in the query job queue to be processed
        bool closed = queryJobQueue_.OperationsFinishedAsyncEvent.WaitOne(maxCloseWaitDuration);
        ASSERT_IFNOT(closed, "{0}: the query job queue didn't close in the alloted time {1}", replica_.PartitionedReplicaId.TraceId, maxCloseWaitDuration);
        replica_.WriteInfo(TraceComponent, "{0}: {1} completed close", replica_.PartitionedReplicaId.TraceId, queryJobQueue_.Name);

        // Wait for all items in the completed job queue to be processed
        closed = completeRequestJobQueue_.OperationsFinishedAsyncEvent.WaitOne(maxCloseWaitDuration);
        ASSERT_IFNOT(closed, "{0}: the completed requests job queue didn't close in the alloted time {1}", replica_.PartitionedReplicaId.TraceId, maxCloseWaitDuration);
        replica_.WriteInfo(TraceComponent, "{0}: {1} completed close", replica_.PartitionedReplicaId.TraceId, completeRequestJobQueue_.Name);
    }

private:
    template <class JobItemType>
    class HMJobQueueBase : public Common::JobQueue<JobItemType, HealthManagerReplica>
    {
        DENY_COPY(HMJobQueueBase)
    public:
        HMJobQueueBase(std::wstring const & name, HealthManagerReplica & replica, int maxThreads = 0, Common::JobQueuePerfCountersSPtr const & perfCounters = nullptr, uint64 queueSize = UINT64_MAX)
            : JobQueue<JobItemType, HealthManagerReplica>(
                name + L"." + replica.PartitionedReplicaId.TraceId,
                replica,
                false /*forceEnqueue*/,
                maxThreads,
                perfCounters,
                queueSize,
                DequePolicy::FifoLifo)
            , onFinishEvent_()
        {
        }

        __declspec(property(get = get_OperationsFinishedEvent)) Common::ManualResetEvent & OperationsFinishedAsyncEvent;
        Common::ManualResetEvent & get_OperationsFinishedEvent() { return onFinishEvent_; }

    protected:
        void OnFinishItems() override { onFinishEvent_.Set(); }

    private:
        Common::ManualResetEvent onFinishEvent_;
    };

    class QueryJobItem : public Common::JobItem<HealthManagerReplica>
    {
    public:
        QueryJobItem()
            : Common::JobItem<HealthManagerReplica>()
            , queryOperation_()
            , queryProcessor_()
        {
        }

        QueryJobItem(
            Common::AsyncOperationSPtr const & queryOperation,
            __in IQueryProcessor * queryProcessor)
            : Common::JobItem<HealthManagerReplica>()
            , queryOperation_(queryOperation)
            , queryProcessor_(queryProcessor)
        {
        }

        QueryJobItem & operator=(QueryJobItem const & other)
        {
            if (this != &other)
            {
                queryOperation_ = other.queryOperation_;
                queryProcessor_ = other.queryProcessor_;
            }

            return *this;
        }

        QueryJobItem(QueryJobItem && other)
            : queryOperation_(move(other.queryOperation_))
            , queryProcessor_(other.queryProcessor_)
        {
        }

        QueryJobItem & operator=(QueryJobItem && other)
        {
            if (this != &other)
            {
                queryOperation_ = move(other.queryOperation_);
                queryProcessor_ = other.queryProcessor_;
            }

            return *this;
        }

        void Process(HealthManagerReplica & /*root*/)
        {
            queryProcessor_->StartProcessQuery(queryOperation_);
        }

        static bool NeedThrottle(HealthManagerReplica & healthManagerReplica)
        {
            // Wait until the cache manager is completely loaded before servicing any requests
            return !healthManagerReplica.EntityManager->IsReady();
        }

    private:
        Common::AsyncOperationSPtr queryOperation_;
        IQueryProcessor * queryProcessor_;
    };

    class CompleteRequestJobItem : public Common::JobItem<HealthManagerReplica>
    {
        DENY_COPY(CompleteRequestJobItem)
    public:
        CompleteRequestJobItem()
            : context_()
        {
        }

        CompleteRequestJobItem(CompleteRequestJobItem && other)
            : context_(move(other.context_))
        {
        }

        CompleteRequestJobItem & operator=(CompleteRequestJobItem && other)
        {
            if (this != &other)
            {
                context_ = move(other.context_);
            }

            return *this;
        }

        CompleteRequestJobItem(std::unique_ptr<RequestContext> && context, Common::ErrorCode && error)
            : context_(move(context))
        {
            context_->SetError(move(error));
        }

        void Process(HealthManagerReplica & /*root*/)
        {
            context_->OnRequestCompleted();
        }

    private:
        std::unique_ptr<RequestContext> context_;
    };

private:
    HealthManagerReplica & replica_;

    // Job queue that handles query requests
    HMJobQueueBase<QueryJobItem> queryJobQueue_;

    // Job queue used to complete reports and sequence streams by notifying the async operation of the results
    HMJobQueueBase<CompleteRequestJobItem> completeRequestJobQueue_;
};

// **********************************
// Internal class - UpdateClusterHealthPolicyAsyncOperation
// **********************************
class HealthManagerReplica::UpdateClusterHealthPolicyAsyncOperation : public AsyncOperation
{
public:
    UpdateClusterHealthPolicyAsyncOperation(
        HealthManagerReplica & owner,
        ActivityId const & activityId,
        ServiceModel::ClusterHealthPolicySPtr const & policy,
        ServiceModel::ClusterUpgradeHealthPolicySPtr const & upgradePolicy,
        ServiceModel::ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , activityId_(activityId)
        , policy_(policy)
        , upgradePolicy_(upgradePolicy)
        , applicationHealthPolicies_(applicationHealthPolicies)
        , timeout_(timeout)
        , clusterEntity_()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<UpdateClusterHealthPolicyAsyncOperation>(operation)->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error = owner_.GetClusterEntity(activityId_, clusterEntity_);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        auto operation = clusterEntity_->BeginUpdateClusterHealthPolicy(
            activityId_,
            policy_,
            upgradePolicy_,
            applicationHealthPolicies_,
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->OnUpdatePolicyComplete(operation, false); },
            thisSPtr);
        this->OnUpdatePolicyComplete(operation, true);
    }

private:
    void OnUpdatePolicyComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;
        auto error = clusterEntity_->EndUpdateClusterHealthPolicy(operation);
        this->TryComplete(thisSPtr, move(error));
    }

    HealthManagerReplica & owner_;
    ActivityId const & activityId_;
    shared_ptr<ClusterHealthPolicy> const & policy_;
    shared_ptr<ClusterUpgradeHealthPolicy> const & upgradePolicy_;
    ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies_;
    TimeSpan timeout_;
    ClusterEntitySPtr clusterEntity_;
};

HealthManagerReplica::HealthManagerReplica(
    __in Store::KeyValueStoreReplica & kvs,
    Guid partitionId,
    FABRIC_REPLICA_ID replicaId,
    __in FederationSubsystem & federation,
    ComponentRoot const & root)
    : PartitionedReplicaTraceComponent(Store::PartitionedReplicaId(partitionId, replicaId))
    , RootedObject(root)
    , ComponentRoot()
    , kvsReplica_(kvs)
    , fabricRoot_(root.CreateComponentRoot())
    , federation_(federation)
    , messageFilterSPtr_()
    , storeWrapper_()
    , jobQueueManager_()
    , jobQueueHolder_()
    , entityManager_()
    , queryMessageHandler_()
    , acceptRequests_(false)
    , healthManagerCounters_()
{
    HMEvents::Trace->ReplicaCtor(
        this->PartitionedReplicaId.TraceId,
        federation_.Instance,
        reinterpret_cast<__int64>(static_cast<void*>(this)));
}

HealthManagerReplica::~HealthManagerReplica()
{
    HMEvents::Trace->ReplicaDtor(
        this->PartitionedReplicaId.TraceId,
        reinterpret_cast<__int64>(static_cast<void*>(this)));
}

// *****************
// IHealthAggregator
// *****************

AsyncOperationSPtr HealthManagerReplica::BeginUpdateApplicationsHealth(
    ActivityId const & activityId,
    vector<HealthReport> && reports,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto body = Common::make_unique<ReportHealthMessageBody>(move(reports), vector<SequenceStreamInformation>());

    return AsyncOperation::CreateAndStart<ProcessReportRequestAsyncOperation>(
        *entityManager_,
        this->PartitionedReplicaId,
        activityId,
        HealthManagerTcpMessage::GetReportHealth(activityId, move(body))->GetTcpMessage(),
        RequestReceiverContextUPtr(),
        timeout,
        callback,
        parent);
}

ErrorCode HealthManagerReplica::EndUpdateApplicationsHealth(
    AsyncOperationSPtr const & operation)
{
    return ProcessReportRequestAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr HealthManagerReplica::BeginUpdateClusterHealthPolicy(
    Common::ActivityId const & activityId,
    ServiceModel::ClusterHealthPolicySPtr const & healthPolicy,
    ServiceModel::ClusterUpgradeHealthPolicySPtr const & updateHealthPolicy,
    ServiceModel::ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpdateClusterHealthPolicyAsyncOperation>(
        *this,
        activityId,
        healthPolicy,
        updateHealthPolicy,
        applicationHealthPolicies,
        timeout,
        callback,
        parent);
}

Common::ErrorCode HealthManagerReplica::EndUpdateClusterHealthPolicy(
    Common::AsyncOperationSPtr const & operation)
{
    return UpdateClusterHealthPolicyAsyncOperation::End(operation);
}

ErrorCode HealthManagerReplica::IsApplicationHealthy(
    ActivityId const & activityId,
    std::wstring const & applicationName,
    vector<wstring> const & upgradeDomains,
    ApplicationHealthPolicy const *policy,
    __inout bool & result,
    __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations) const
{
    this->HealthManagerCounters->OnHealthQueryReceived();

    HealthEntitySPtr entity;
    ErrorCode error = GetApplicationEntity(activityId, applicationName, entity);
    if (!error.IsSuccess()) { return error; }

    auto application = ApplicationsCache::GetCastedEntityPtr(entity);
    error = application->IsApplicationHealthy(activityId, policy, upgradeDomains, result, unhealthyEvaluations);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceUpgrade,
            "{0}:{1} app {2} health evaluation failed: {3}",
            this->PartitionedReplicaId.TraceId,
            activityId,
            applicationName,
            error);

        return error;
    }

    return error;
}

Common::ErrorCode HealthManagerReplica::GetClusterUpgradeSnapshot(
    Common::ActivityId const & activityId,
    __inout ClusterUpgradeStateSnapshot & snapshot) const
{
    ClusterEntitySPtr cluster;
    ErrorCode error = GetClusterEntity(activityId, cluster);
    if (!error.IsSuccess()) { return error; }

    return cluster->GetClusterUpgradeSnapshot(activityId, snapshot);
}

Common::ErrorCode HealthManagerReplica::AppendToClusterUpgradeSnapshot(
    Common::ActivityId const & activityId,
    std::vector<std::wstring> const & upgradeDomains,
    __inout ClusterUpgradeStateSnapshot & snapshot) const
{
    ClusterEntitySPtr cluster;
    ErrorCode error = GetClusterEntity(activityId, cluster);
    if (!error.IsSuccess()) { return error; }

    return cluster->AppendToClusterUpgradeSnapshot(activityId, upgradeDomains, snapshot);
}

ErrorCode HealthManagerReplica::Test_IsClusterHealthy(
    ActivityId const & activityId,
    vector<wstring> const & upgradeDomains,
    ServiceModel::ClusterHealthPolicySPtr const & policy,
    ServiceModel::ClusterUpgradeHealthPolicySPtr const & upgradePolicy,
    ServiceModel::ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies,
    ClusterUpgradeStateSnapshot const & baseline,
    __inout bool & result,
    __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
    __inout std::vector<std::wstring> & applicationsWithoutAppType) const
{
    this->HealthManagerCounters->OnHealthQueryReceived();

    result = false;
    ClusterEntitySPtr cluster;
    ErrorCode error = GetClusterEntity(activityId, cluster);
    if (!error.IsSuccess()) { return error; }

    FABRIC_HEALTH_STATE aggregatedHealthState;
    // Evaluate health and get unhealthy evaluations (if any) without description.
    error = cluster->EvaluateHealth(activityId, upgradeDomains, policy, upgradePolicy, applicationHealthPolicies, baseline, aggregatedHealthState, unhealthyEvaluations, applicationsWithoutAppType);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceUpgrade,
            "{0}:{1} cluster health evaluation failed: {2}",
            this->PartitionedReplicaId.TraceId,
            activityId,
            error);

        return error;
    }

    size_t currentSize = 0;
    error = HealthEvaluation::GenerateDescriptionAndTrimIfNeeded(
        unhealthyEvaluations,
        ManagementConfig::GetConfig().MaxUnhealthyEvaluationsSizeBytes,
        currentSize);
    if (error.IsSuccess())
    {
        if (aggregatedHealthState == FABRIC_HEALTH_STATE_ERROR)
        {
            TESTASSERT_IF(unhealthyEvaluations.empty(), "{0}:{1} cluster is not healthy but evaluation reasons are not set", this->PartitionedReplicaId.TraceId, activityId);
            HMEvents::Trace->UpgradeClusterUnhealthy(
                this->PartitionedReplicaId.TraceId,
                activityId,
                wformatString(aggregatedHealthState),
                *unhealthyEvaluations[0].Evaluation);
        }
        else
        {
            if (unhealthyEvaluations.empty())
            {
                HMEvents::Trace->ClusterHealthy(
                    this->PartitionedReplicaId.TraceId,
                    activityId,
                    wformatString(aggregatedHealthState));
            }
            else
            {
                HMEvents::Trace->ClusterHealthyWithEvaluations(
                    this->PartitionedReplicaId.TraceId,
                    activityId,
                    wformatString(aggregatedHealthState),
                    *unhealthyEvaluations[0].Evaluation);
            }

            // Consider OK and Warning as success, do not return evaluation reasons
            result = true;
            unhealthyEvaluations.clear();
        }
    }
    else
    {
        // Trace, but do not return error.
        // Upgrade can continue even if the unhealthy evaluations are empty.
        WriteWarning(
            TraceUpgrade,
            "{0}: {1}: generating description for the unhealthy evaluations failed: {2}. ",
            activityId,
            aggregatedHealthState,
            error);
    }

    return ErrorCode::Success();
}

ErrorCode HealthManagerReplica::IsClusterHealthy(
    ActivityId const & activityId,
    vector<wstring> const & upgradeDomains,
    ClusterUpgradeStateSnapshot const & baseline,
    __inout bool & result,
    __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
    __inout std::vector<std::wstring> & applicationsWithoutAppType) const
{
    return Test_IsClusterHealthy(activityId, upgradeDomains, nullptr, nullptr, nullptr, baseline, result, unhealthyEvaluations, applicationsWithoutAppType);
}

// **********************************
// State changes
// **********************************
ErrorCode HealthManagerReplica::OnOpen(
    __in Store::IReplicatedStore * store,
    SystemServiceMessageFilterSPtr const & messageFilter)
{
    HMEvents::Trace->ReplicaOpen(
        this->PartitionedReplicaId.TraceId,
        this->NodeInstance);

    ErrorCode error(ErrorCodeValue::Success);

    // The state changes are coming from CM Replica,
    // Te service replica ensures that state changes are called sequentially,
    // so there is no need to protect changes with a lock

    // Create the replicated store wrapper to be able to access the replicated store
    storeWrapper_ = make_unique<ReplicatedStoreWrapper>(store, this->PartitionedReplicaId);

    healthManagerCounters_ = HealthManagerCounters::CreateInstance(
        this->PartitionedReplicaId.TraceId + L":" + StringUtility::ToWString(DateTime::Now().Ticks));

    // Create the entity manager that will process incoming reports
    // while the replica is primary.
    // The entity manager keeps the current replica alive, which in turn uses the fabric root.
    entityManager_ = make_unique<EntityCacheManager>(*this, *this);

    // Make a copy of the message filter and use it to filter incoming messages based on the service location
    messageFilterSPtr_ = messageFilter;
    error = RegisterMessageHandler();
    if (!error.IsSuccess()) { return error; }

    jobQueueHolder_ = make_unique<JobQueueHolder>(*this);

    jobQueueManager_ = make_unique<EntityJobQueueManager>(
        this->PartitionedReplicaId,
        *this,
        ManagementConfig::GetConfig().HealthProcessingQueueThreadCount);

    acceptRequests_.store(true);
    error = entityManager_->Open();

    return error;
}

ErrorCode HealthManagerReplica::OnClose()
{
    HMEvents::Trace->ReplicaClose(this->PartitionedReplicaId.TraceId);
    ErrorCode error(ErrorCodeValue::Success);

    acceptRequests_.store(false);

    // Best-effort close, so proceed instead of returning error if the handler is not unregistered
    TryUnregisterMessageHandler();

    if (entityManager_)
    {
        error = entityManager_->Close();
        WriteInfo(TraceComponent, "{0}: entity manager completed close with {1}", this->PartitionedReplicaId.TraceId, error);
    }

    if (jobQueueManager_)
    {
        jobQueueManager_->Close();
        WriteInfo(TraceComponent, "{0}: job queue manager completed close", this->PartitionedReplicaId.TraceId);
    }

    if (jobQueueHolder_)
    {
        jobQueueHolder_->Close();
        WriteInfo(TraceComponent, "{0}: QueryJobQueue completed close", this->PartitionedReplicaId.TraceId);
    }

    HMEvents::Trace->ReplicaCloseCompleted(this->PartitionedReplicaId.TraceId, error);
    return error;
}

void HealthManagerReplica::OnAbort()
{
    HMEvents::Trace->ReplicaAbort(this->PartitionedReplicaId.TraceId);
    OnClose();
}

// *******************************************
// Helper methods
// *******************************************
void HealthManagerReplica::ReportTransientFault(Common::ErrorCode const & fatalError)
{
    kvsReplica_.TransientFault(wformatString("error={0} msg='{1}'", fatalError, fatalError.Message));
}

 Common::ErrorCode HealthManagerReplica::StartCommitJobItem(
    IHealthJobItemSPtr const & jobItem,
    PrepareTransactionCallback const & prepareTxCallback,
    TransactionCompletedCallback const & txCompletedCallback,
    Common::TimeSpan const timeout)
 {
    if (timeout <= TimeSpan::Zero)
    {
        return ErrorCode(ErrorCodeValue::Timeout);
    }

    auto inner = ReplicatedStoreWrapper::BeginCommit(
        jobItem->ReplicaActivityId,
        prepareTxCallback,
        [jobItem, this] (AsyncOperationSPtr const & operation)
        {
            if (!operation->CompletedSynchronously)
            {
                this->JobQueueManager.OnAsyncWorkCompleted(jobItem, operation);
            }
        },
        this->CreateAsyncOperationRoot());

    // Based on commit results, update in memory data (on success),
    // notify caller and schedule more work
    if (!inner->CompletedSynchronously)
    {
        // Notify the job item that async work is started
        this->JobQueueManager.OnAsyncWorkStarted(*jobItem);
    }
    else
    {
        txCompletedCallback(*jobItem, inner);
    }

    return ErrorCode::Success();
}

 void HealthManagerReplica::OnCacheLoadCompleted()
 {
     ASSERT_IFNOT(jobQueueHolder_, "{0}: jobQueueHolder_ must exist on all caches completed", this->PartitionedReplicaId.TraceId);
     jobQueueHolder_->OnCacheLoadCompleted();
 }

 Common::ErrorCode HealthManagerReplica::AddQuery(
     Common::AsyncOperationSPtr const & queryOperation,
     __in IQueryProcessor * queryProcessor)
 {
     ASSERT_IFNOT(jobQueueHolder_, "{0}: jobQueueHolder_ must exist on AddQuery", this->PartitionedReplicaId.TraceId);
     return jobQueueHolder_->AddQuery(queryOperation, queryProcessor);
 }

 void HealthManagerReplica::QueueReportForCompletion(ReportRequestContext && context, Common::ErrorCode && error)
 {
     ASSERT_IFNOT(jobQueueHolder_, "{0}: jobQueueHolder_ must exist on QueueReportForCompletion", this->PartitionedReplicaId.TraceId);
     return jobQueueHolder_->QueueReportForCompletion(move(context), move(error));
 }

 void HealthManagerReplica::QueueSequenceStreamForCompletion(SequenceStreamRequestContext && context, Common::ErrorCode && error)
 {
     ASSERT_IFNOT(jobQueueHolder_, "{0}: jobQueueHolder_ must exist on QueueSequenceStreamForCompletion", this->PartitionedReplicaId.TraceId);
     return jobQueueHolder_->QueueSequenceStreamForCompletion(move(context), move(error));
 }

// **********************************
// Message processing
// **********************************
ErrorCode HealthManagerReplica::RegisterMessageHandler()
{
    ErrorCode error(ErrorCodeValue::Success);
    auto selfRoot = this->CreateComponentRoot();

    // Create and register the query handlers
    queryMessageHandler_ = make_unique<QueryMessageHandler>(
        *selfRoot,
        QueryAddresses::HMAddressSegment);

    queryMessageHandler_->RegisterQueryHandler(
        [this](QueryNames::Enum queryName, QueryArgumentMap const & queryArgs, Common::ActivityId const & activityId, TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
        {
            return this->BeginProcessQuery(queryName, queryArgs, activityId, timeout, callback, parent);
        },
        [this](Common::AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
        {
            return this->EndProcessQuery(operation, reply);
        });

    error = queryMessageHandler_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0}: QueryMessageHandler failed to open: error = {1}",
            this->PartitionedReplicaId.TraceId,
            error);
        return error;
    }

    auto filter = messageFilterSPtr_;

    // Process incoming messages on the transport threads, because the initial processing is minimal:
    // the requests are given to the job queue manager, which will process them when threads are available.
    // When done, the processing of the message will be continued, so there is no blocking of the original transport threads.
    federation_.RegisterMessageHandler(
        Transport::Actor::HM,
        [this](MessageUPtr & request, OneWayReceiverContextUPtr & requestContext)
        {
            WriteError(
                TraceComponent,
                "{0} received a oneway message: {1}",
                this->PartitionedReplicaId.TraceId,
                *request);
            requestContext->Reject(ErrorCodeValue::InvalidMessage);
        },
        [this, selfRoot](MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext)
        {
            this->RequestMessageHandler(move(message), move(requestReceiverContext));
        },
        true/*dispatchOnTransportThread*/,
        move(filter));

    return error;
}

bool HealthManagerReplica::TryUnregisterMessageHandler()
{
    bool success = federation_.UnRegisterMessageHandler(
        Transport::Actor::HM,
        messageFilterSPtr_);

    if (!success)
    {
        WriteInfo(
            TraceComponent,
            "{0}: unable to unregister message handler",
            this->PartitionedReplicaId.TraceId);
    }

    if (queryMessageHandler_)
    {
        queryMessageHandler_->Close();
    }

    return success;
}

Common::AsyncOperationSPtr HealthManagerReplica::BeginProcessQuery(
    Query::QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs,
    Common::ActivityId const & activityId,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    ErrorCode error(ErrorCodeValue::Success);
    if (!acceptRequests_.load())
    {
        error = ErrorCode(ErrorCodeValue::ObjectClosed);
    }

    return AsyncOperation::CreateAndStart<ProcessInnerQueryRequestAsyncOperation>(
        *entityManager_,
        this->PartitionedReplicaId,
        activityId,
        error,
        queryName,
        queryArgs,
        timeout,
        callback,
        parent);
}

Common::ErrorCode HealthManagerReplica::EndProcessQuery(
    Common::AsyncOperationSPtr const & operation,
    __out Transport::MessageUPtr & reply)
{
    return ProcessInnerQueryRequestAsyncOperation::End(operation, reply);
}

void HealthManagerReplica::RequestMessageHandler(
    MessageUPtr && request,
    RequestReceiverContextUPtr && requestContext)
{
    FabricActivityHeader activityHeader;
    if (!request->Headers.TryReadFirst(activityHeader))
    {
        HMEvents::Trace->InvalidMessage(
            this->PartitionedReplicaId.TraceId,
            request->Action,
            request->MessageId,
            L"ActivityHeader");
        requestContext->Reject(ErrorCodeValue::InvalidMessage);
        return;
    }

    auto activityId = activityHeader.ActivityId;

    if (request->Actor != Transport::Actor::HM)
    {
        HMEvents::Trace->InvalidActor(
            this->PartitionedReplicaId.TraceId,
            activityId,
            request->Action,
            request->MessageId,
            wformatString(request->Actor));
        requestContext->Reject(ErrorCodeValue::InvalidMessage);
        return;
    }

    TimeoutHeader timeoutHeader;
    if (!request->Headers.TryReadFirst(timeoutHeader))
    {
        HMEvents::Trace->InvalidMessage2(
            this->PartitionedReplicaId.TraceId,
            activityId,
            request->Action,
            request->MessageId,
            L"TimeoutHeader");
        requestContext->Reject(ErrorCodeValue::InvalidMessage);
        return;
    }

    auto timeout = timeoutHeader.Timeout;
    auto messageId = request->MessageId;
    HMEvents::Trace->RequestReceived(
        this->PartitionedReplicaId.TraceId,
        activityId,
        request->Action,
        messageId);

    // Processing can continue asynchronously after unregistering the message handler
    auto selfRoot = this->CreateComponentRoot();

    StartProcessRequest(
        move(request),
        move(requestContext),
        activityId,
        timeout,
        [this, selfRoot, activityId, messageId](AsyncOperationSPtr const & asyncOperation) mutable
        {
            this->OnProcessRequestComplete(activityId, messageId, asyncOperation);
        },
        this->Root.CreateAsyncOperationRoot());
}

AsyncOperationSPtr HealthManagerReplica::StartProcessRequest(
    MessageUPtr && request,
    RequestReceiverContextUPtr && requestContext,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ErrorCode error(ErrorCodeValue::Success);

    if (!acceptRequests_.load())
    {
        error = ErrorCodeValue::NotPrimary;
        HMEvents::Trace->RequestRejected(
            this->PartitionedReplicaId.TraceId,
            activityId,
            request->MessageId,
            error);
    }

    if (error.IsSuccess())
    {
        if (request->Action == HealthManagerTcpMessage::ReportHealthAction)
        {
            return AsyncOperation::CreateAndStart<ProcessReportRequestAsyncOperation>(
                *entityManager_,
                this->PartitionedReplicaId,
                activityId,
                move(request),
                move(requestContext),
                timeout,
                callback,
                parent);
        }
        else if (request->Action == HealthManagerTcpMessage::GetSequenceStreamProgressAction)
        {
           return AsyncOperation::CreateAndStart<GetSequenceStreamProgressAsyncOperation>(
              *entityManager_,
              this->PartitionedReplicaId,
              activityId,
              move(request),
              move(requestContext),
              timeout,
              callback,
              parent);
        }
        else if (request->Action == QueryTcpMessage::QueryAction)
        {
           return AsyncOperation::CreateAndStart<ProcessQueryRequestAsyncOperation>(
              *entityManager_,
              this->PartitionedReplicaId,
              activityId,
              queryMessageHandler_,
              move(request),
              move(requestContext),
              timeout,
              callback,
              parent);
        }
        else
        {
            HMEvents::Trace->InvalidAction(
                this->PartitionedReplicaId.TraceId,
                activityId,
                request->Action,
                request->MessageId);

            error = ErrorCodeValue::InvalidMessage;
        }
    }

     return AsyncOperation::CreateAndStart<ProcessRequestAsyncOperation>(
        *entityManager_,
        this->PartitionedReplicaId,
        activityId,
        move(error),
        move(request),
        move(requestContext),
        timeout,
        callback,
        parent);
}

void HealthManagerReplica::OnProcessRequestComplete(
    Common::ActivityId const & activityId,
    MessageId const & messageId,
    AsyncOperationSPtr const & asyncOperation)
{
    MessageUPtr reply;
    RequestReceiverContextUPtr requestContext;
    ErrorCode error = ProcessRequestAsyncOperation::End(asyncOperation, reply, requestContext);

    if (error.IsSuccess())
    {
        HMEvents::Trace->RequestSucceeded(
            this->PartitionedReplicaId.TraceId,
            activityId,
            messageId);

        reply->Headers.Replace(FabricActivityHeader(activityId));
        requestContext->Reply(std::move(reply));
    }
    else
    {
        HMEvents::Trace->RequestFailed(
            this->PartitionedReplicaId.TraceId,
            activityId,
            messageId,
            error);

        requestContext->Reject(ToNamingGatewayReply(error));
    }
}

Common::ErrorCode HealthManagerReplica::GetClusterEntity(
    Common::ActivityId const & activityId,
    __inout ClusterEntitySPtr & cluster) const
{
    if (!entityManager_->IsReady())
    {
        return ErrorCodeValue::NotReady;
    }

    cluster = this->entityManager_->Cluster.ClusterObj;
    if (!cluster || cluster->IsDeletedOrCleanedUp())
    {
        TRACE_WARNING_AND_TESTASSERT(TraceComponent, "{0}:{1} cluster entity not found", this->PartitionedReplicaId.TraceId, activityId);
        return ErrorCodeValue::InvalidState;
    }

    return ErrorCode::Success();
}

Common::ErrorCode HealthManagerReplica::GetApplicationEntity(
    Common::ActivityId const & activityId,
    std::wstring const & applicationName,
    __inout HealthEntitySPtr & entity) const
{
    if (!entityManager_->IsReady())
    {
        return ErrorCodeValue::NotReady;
    }

    entity = this->entityManager_->Applications.GetEntity(
        ApplicationHealthId(applicationName));
    if (!entity || entity->IsDeletedOrCleanedUp())
    {
        WriteInfo(
            TraceUpgrade,
            "{0}:{1}: application {2} not found for upgrade",
            this->PartitionedReplicaId.TraceId,
            activityId,
            applicationName);
        return ErrorCodeValue::HealthEntityNotFound;
    }

    return ErrorCode::Success();
}

AsyncOperationSPtr HealthManagerReplica::Test_BeginProcessRequest(
    MessageUPtr && request,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return StartProcessRequest(
        move(request),
        nullptr /*requestContext*/,
        activityId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode HealthManagerReplica::Test_EndProcessRequest(
    AsyncOperationSPtr const & operation,
    __out Transport::MessageUPtr & reply)
{
    RequestReceiverContextUPtr requestContext;
    return ProcessRequestAsyncOperation::End(operation, reply, requestContext);
}

bool HealthManagerReplica::Test_IsOpened() const
{
    return acceptRequests_.load();
}

ErrorCode HealthManagerReplica::ToNamingGatewayReply(Common::ErrorCode const & error)
{
    switch (error.ReadValue())
    {
        // Convert these error codes to an error that can be retried by the Naming Gateway
        case ErrorCodeValue::ObjectClosed:
        case ErrorCodeValue::InvalidState:
            return ErrorCodeValue::ReconfigurationPending;

        default:
            return error;
    }
}
