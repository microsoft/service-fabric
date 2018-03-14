// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::AcquireReadLock;
using Common::AcquireWriteLock;
using Common::AsyncOperationSPtr;
using Common::AsyncCallback;
using Common::ComPointer;
using Common::CompletedAsyncOperation;
using Common::ErrorCode;
using Common::FabricComponent;
using Common::FabricComponentState;
using Common::AsyncManualResetEvent;
using Common::StringUtility;
using Common::StringWriter;
using Common::TimeSpan;
using Common::JobQueue;
using Common::ComponentRoot;

using std::unordered_map;
using std::move;
using std::vector;
using std::wstring;

using Common::Assert;

struct AsyncOperationSPtrJob
{
public:
    AsyncOperationSPtrJob()
    {
    }

    explicit AsyncOperationSPtrJob(AsyncOperationSPtr && operation)
        : operation_(move(operation))
    {
    }

    AsyncOperationSPtrJob(AsyncOperationSPtrJob && other)
        : operation_(move(other.operation_))
    {
    }

    AsyncOperationSPtrJob & operator=(AsyncOperationSPtrJob && other)
    {
        if (this == &other)
        {
            return *this;
        }

        operation_ = move(other.operation_);
        return *this;
    }
    
    bool ProcessJob(ReplicaManager & replicaManager)
    {
        UNREFERENCED_PARAMETER(replicaManager);

        operation_->TryComplete(operation_);

        return true;
    }

private:
    AsyncOperationSPtr operation_;
};

class ReplicaManager::CompleteReplicateJobQueue
    : public JobQueue<AsyncOperationSPtrJob, ReplicaManager>
{
    DENY_COPY(CompleteReplicateJobQueue);

protected:
    void OnFinishItems()
    {
        onFinishEvent_.Set();
    }
public:
    static ErrorCode Create(std::wstring const & name, ReplicaManager & root, int threadCount, _Out_ CompleteReplicateJobQueueUPtr & ptr)
    {
        ptr = std::unique_ptr<CompleteReplicateJobQueue>(new CompleteReplicateJobQueue(name, root, threadCount));
        return ErrorCode();
    }

    __declspec(property(get=get_OperationsFinishedEvent)) AsyncManualResetEvent & OperationsFinishedAsyncEvent;
    AsyncManualResetEvent & get_OperationsFinishedEvent() { return onFinishEvent_; }

private:
    CompleteReplicateJobQueue(std::wstring const & name, ReplicaManager & root, int threadCount)
        : JobQueue(name, root, false, threadCount),
        onFinishEvent_()
    {
    }

    // This should be a manual reset event to account for cases when an abort is followed after a close.
    // Since both these methods wait for the queue to finish draining, this event should be SET permanently rather than being reset after 1 thread release
    AsyncManualResetEvent onFinishEvent_;
};

struct ComReplicateJobPtr
{
    ComReplicateJobPtr()
        : operation_()
        , sessions_()
        , completedSeqNumber_()
    {
    }

    explicit ComReplicateJobPtr(ComOperationRawPtr && operation, vector<ReplicationSessionSPtr> && sessions, FABRIC_SEQUENCE_NUMBER completedSeqNumber)
        : operation_(move(operation))
        , sessions_(move(sessions))
        , completedSeqNumber_(completedSeqNumber)
    {
    }
    
    bool ProcessJob(ReplicaManager & replicaManager)
    {
        UNREFERENCED_PARAMETER(replicaManager);

        // Multicast the operation to all replicas (Active & idle)
        // The operation will not be completed until 
        // all replicas send ACK
        ReplicatorEventSource::Events->PrimaryReplicate(
            replicaManager.EndpointUniqueId.PartitionId,
            replicaManager.EndpointUniqueId,
            operation_->SequenceNumber,
            static_cast<uint64>(sessions_.size()));
        
        for(auto const &r: sessions_)
        {
            r->AddReplicateOperation(operation_, completedSeqNumber_);
        }

        return true;
    }

private:
    ComOperationRawPtr operation_;
    vector<ReplicationSessionSPtr> sessions_;
    FABRIC_SEQUENCE_NUMBER completedSeqNumber_;
};

class ReplicaManager::BeginReplicateJobQueue
    : public JobQueue<ComReplicateJobPtr, ReplicaManager>
{
    DENY_COPY(BeginReplicateJobQueue);

public:
    static void Create(std::wstring const & name, ReplicaManager & root, int threadCount, _Out_ BeginReplicateJobQueueUPtr & ptr)
    {
        ptr = std::unique_ptr<BeginReplicateJobQueue>(new BeginReplicateJobQueue(name, root, threadCount));
    }

private:
    BeginReplicateJobQueue(std::wstring const & name, ReplicaManager & root, int threadCount)
        : JobQueue(name, root, false, threadCount)
    {
    }
};

// Static Helper methods
class ReplicaManager::Helpers
{
public:

    static ReplicationSessionVector GetActiveReplicasThatAreNotFaultedDueToSlowProgress(ReplicationSessionVector const & input)
    {
        ReplicationSessionVector result;
        for (auto const & r : input)
        {
            if (!r->IsActiveFaultedDueToSlowProgress)
            {
                result.push_back(r);
            }
        }

        return result;
    }

    static ReplicationSessionVector AscendingSortReplicaProgress(ReplicationSessionVector const & replicas, bool useReceiveAck)
    {
        auto ackSorted = replicas;
        std::sort(ackSorted.begin(), ackSorted.end(),
            [&](ReplicationSessionSPtr const & left, ReplicationSessionSPtr const & right)
        {
            if (useReceiveAck)
            {
                return left->GetLastReceiveLsn() < right->GetLastReceiveLsn();
            }
            else
            {
                return left->GetLastApplyLsn() < right->GetLastApplyLsn();
            }
        });
        
        return ackSorted;
    }

    static ReplicationSessionVector AscendingSortAckDuration(ReplicationSessionVector const & replicas, bool useReceiveAck)
    {
        auto ackSorted = replicas;
        std::sort(ackSorted.begin(), ackSorted.end(),
            [&](ReplicationSessionSPtr const & left, ReplicationSessionSPtr const & right)
        {
            if (useReceiveAck)
            {
                return left->AvgReceiveAckDuration < right->AvgReceiveAckDuration;
            }
            else
            {
                return left->AvgApplyAckDuration < right->AvgApplyAckDuration;
            }
        });
        
        return ackSorted;
    }

    static StandardDeviation ComputeAverageAckDuration(
        ReplicationSessionVector const & replicas,
        size_t uptoIndex,
        bool useReceiveAck)
    {
        StandardDeviation sd;

        // This condition should never happen and hence adding a test assert and adjusting product code below to 
        // not crash even if this is not true
        TESTASSERT_IF(uptoIndex >= replicas.size(), "uptoIndex={0} and replica count is {1}", uptoIndex, replicas.size());

        for (auto i = 0; i < replicas.size(); i++)
        {
            if (i > uptoIndex)
                break;
            
            if (useReceiveAck)
            {
                sd.Add(replicas[i]->AvgReceiveAckDuration);
            }
            else
            {
                sd.Add(replicas[i]->AvgApplyAckDuration);
            }
        }

        return sd;
    }

    // A replica is considered slow if its ack duration is greater than 2*StdDev + Mean of other replicas in the quorum
    static ReplicationSessionSPtr GetSlowestReplicaIfPresent(
        ReplicationSessionVector const & replicas,
        ULONG writeQuorum,
        StandardDeviation & avgDuration)
    {
        auto receiveAckDurationSorted = AscendingSortAckDuration(replicas, true /*useReceiveAck*/);
        auto slowestReceiveAckDurationReplica = receiveAckDurationSorted[receiveAckDurationSorted.size() - 1]; // the last item is the highest average duration => slowest replica

        ASSERT_IF(writeQuorum < 2, "GetSlowestReplicaIfPresent expects write quorum to be greater than 1. It is {0}", writeQuorum);

        avgDuration = ComputeAverageAckDuration(
            receiveAckDurationSorted,
            writeQuorum - 2, // primary is part of the quorum, hence won't be in activereplicas. Index of the quorum is 1 minus writeQuorum_ - 1
            /*useReceiveAck*/true);
        
        // If the replica's ack duration is greater than mean+2*sd, it is consisedered slow
        if (slowestReceiveAckDurationReplica->AvgReceiveAckDuration > (avgDuration.Average + avgDuration.StdDev + avgDuration.StdDev))
        {
            auto receiveAckProgressSorted = AscendingSortReplicaProgress(replicas, true /*useReceiveAck*/);
            auto slowestReceiveAckProgressReplica = receiveAckProgressSorted[0]; // the first item is the lowest progress => slowest replica

            // Ensure that the slowest receive ACK duration is for the replica with the least progress

            if (slowestReceiveAckProgressReplica->ReplicaId == slowestReceiveAckDurationReplica->ReplicaId)
            {
                return slowestReceiveAckDurationReplica;
            }
        }

        return nullptr;
    }

    static ReplicationSessionSPtr GetReplicaFromAddress(
        ReplicationSessionVector const & replicas,
        std::wstring const & address,
        ReplicationEndpointId const & endpointUniqueId)
    {
        auto it = std::find_if(replicas.begin(), replicas.end(), 
            [&address, &endpointUniqueId] (ReplicationSessionSPtr const & r)->bool 
            { 
                return r->HasEndpoint(address, endpointUniqueId);
            });

        if (it != replicas.end())
        {
            return *it;
        }

        return nullptr;
    }

    static ReplicationSessionSPtr GetReplica(
        ReplicationSessionVector const & replicas,
        ReplicaInformation const & replica)
    {
        auto it = std::find_if(replicas.begin(), replicas.end(), 
            [&replica] (ReplicationSessionSPtr const & r)->bool 
            { 
                return r->IsSameReplica(replica.Id);
            });

        if (it != replicas.end())
        {
            return *it;
        }

        return nullptr;
    }

    static ReplicaInformationVectorCIter GetReplicaIter(
        ReplicaInformationVector const & replicas,
        ReplicationSessionSPtr const & session)
    {
        auto it = std::find_if(replicas.begin(), replicas.end(), 
            [&session] (ReplicaInformation const & r)->bool 
            { 
                return session->IsSameReplica(r.Id); 
            });
        return it;    
    }

    static wstring GetReplicaIdsString(
        ReplicationSessionVector const & replicas)
    {
        wstring replicasInfo;
        StringWriter writer(replicasInfo);
        for (auto const & r : replicas)
        {
            writer.Write("{0}:{1};", r->ReplicaId, r->GetLastApplyLsn());
        }

        return replicasInfo;
    }

    static void GetReplicasProgress(
        FABRIC_SEQUENCE_NUMBER completedLsn,
        ReplicationSessionVector const & replicas,
        ULONG writeQuorum,
        ReplicationEndpointId const & description,
        __out FABRIC_SEQUENCE_NUMBER & committedLSN,
        __out FABRIC_SEQUENCE_NUMBER & allAckedLSN,
        __out FABRIC_SEQUENCE_NUMBER & completedLSN)
    {
        if (writeQuorum <= 1)
        {
            committedLSN = Constants::NonInitializedLSN;
            completedLSN = Constants::NonInitializedLSN;
            allAckedLSN = Constants::NonInitializedLSN;
        }
        else
        {
            size_t replicaCount = replicas.size();
            // Since the primary is included in quorum and considered as immediately ACKing,
            // the committed number must be ACKed by writeQuorum - 1 remote replicas.
            // To compute the correct number, we sort the vector of LSNs 
            // and take the number at index activeQuorum - 1.
            // Since writeQuorum > 1 => quorum index >= 0
            size_t quorumIndex = static_cast<size_t>(writeQuorum - 2);

            ASSERT_IF(
                quorumIndex >= replicaCount,
                "{0}: GetProgress: should be enough replicas for quorum, but index {1} >= active replica count {2}",
                description,
                quorumIndex,
                replicaCount); 

            // Descending sort the acks, then look at the index 
            // given by quorum index
            vector<FABRIC_SEQUENCE_NUMBER> lastAckedLSNs = GetSortedLSNList(
                completedLsn,
                replicas,
                false /*useReceiveAck*/,
                [](FABRIC_SEQUENCE_NUMBER i, FABRIC_SEQUENCE_NUMBER j) 
                    { return i > j; });
            
            committedLSN = lastAckedLSNs[quorumIndex];
            allAckedLSN = lastAckedLSNs[replicaCount - 1]; 

            // *******************************************NOTE - NOT CONSIDERING SLOW FAULTED REPLICAS FOR QUEUE PROGRESS***************************************************
            // Use only replicas that are not faulted to ensure slow faulted replicas do not cause queue full if the close/report fault on the remote replica is stuck due to any reason

            // The only downside of this approach is that if the secondary ever recovers and we receive ACK's for this replica, we will still ignore its progress.
            // This also means that if there is a catchup call with this secondary being marked as "MustCatchup", the catchup will get stuck forever IF the secondary is not reachable.
            // If the secondary is reachable, it will fault itself and hence failover will cancel the catchup API

            // The other pre-condition in order to be able to perform this optimization is that the replication session must close the reliable operation sender. Without doing so could result in AV
            // because the reliable operation sender could attempt to send a REPL operation that is already free'd up in the queue 

            auto nonFaultedReplicas = GetActiveReplicasThatAreNotFaultedDueToSlowProgress(replicas);

            if (nonFaultedReplicas.size() == 0)
            {
                completedLSN = Constants::NonInitializedLSN;
                return;
            }

            // Ascending sort the receive acks and look at the least one for the completed LSN
            vector<FABRIC_SEQUENCE_NUMBER> lastReceiveAckedLSNs = GetSortedLSNList(
                completedLsn,
                nonFaultedReplicas,
                true /*useReceiveAck*/,
                [](FABRIC_SEQUENCE_NUMBER i, FABRIC_SEQUENCE_NUMBER j) 
                    { return i < j; });
           
            completedLSN = lastReceiveAckedLSNs[0];
            
            // If receive ACK progress of all replicas turns out to be greater than quorum, trim down to the quorum as we may need to re-send these 
            // non committed operations during new build requests
            if (completedLSN > committedLSN)
            {
                completedLSN = committedLSN;
            }
        }
    }

    static vector<FABRIC_SEQUENCE_NUMBER> GetSortedLSNList(
        FABRIC_SEQUENCE_NUMBER completedLsn,
        __in ReplicationSessionVector const & replicas,
        bool useReceiveAck,
        __in std::function < bool(FABRIC_SEQUENCE_NUMBER left, FABRIC_SEQUENCE_NUMBER right)> comparisionFunc)
    {
        vector<FABRIC_SEQUENCE_NUMBER> sortedLSNs;
        
        for (auto const &r : replicas)
        {
            if (useReceiveAck)
            {
                sortedLSNs.push_back(r->GetLastReceiveLsn(completedLsn));
            }
            else
            {
                sortedLSNs.push_back(r->GetLastApplyLsn(completedLsn));
            }
        }

        std::sort(sortedLSNs.begin(), sortedLSNs.end(), comparisionFunc);

        return sortedLSNs;
    }

    static bool RestartSlowActiveSecondaryConfigEnabled(
        __in bool hasPersistedState,
        __in REInternalSettingsSPtr const & config)
    {
        if (hasPersistedState)
        {
            return config->EnableSlowActiveSecondaryRestartForPersisted;
        }
        
        return config->EnableSlowActiveSecondaryRestartForVolatile;
    }

    static bool RestartSlowIdleConfigEnabled(
        __in bool hasPersistedState,
        __in REInternalSettingsSPtr const & config)
    {
        if (hasPersistedState)
        {
            return config->EnableSlowIdleRestartForPersisted;
        }
        
        return config->EnableSlowIdleRestartForVolatile;
    }

    static void CloseReliableOperationSenders(
        __in ReplicationSessionVector & sessions)
    {
        for (auto const & r : sessions)
        {
            r->Close();
        }
    }

    static bool UpdateLastReplicationLSNsForIdleReplicas(
        __in ReplicationSessionVector const & idleReplicas,
        __in FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber)
    {
        ASSERT_IF(idleReplicas.empty(), "UpdateLastReplicationLSNsForIdleReplicas contains no replicas");
        bool ret = false;
        
        for (auto const &r : idleReplicas)
        {
            // Return true if at-least 1 idle replica succesfully updated its replication LSN.
            // This should result in free-ing up the queue
            ret = ret | r->UpdateLastReplicationSequenceNumberForIdle(lastReplicationSequenceNumber);
        }

        return ret;
    }

    static wstring GetHealthReportWarningDescription(
        __in ReplicationSessionVector const & cc,
        __in ReplicationSessionVector const & idle,
        __in ReplicationSessionVector const & pc,
        __in FABRIC_SEQUENCE_NUMBER const & firstLsn,
        __in FABRIC_SEQUENCE_NUMBER const & lastLsn,
        __in uint const & queueUsage,
        __in uint const & queueUsageThreshold)
    {
        std::wstring content;
        Common::StringWriter writer(content);

        writer.WriteLine("Primary Replication Queue Usage of {0}% has reached/exceeded the threshold {1}%. First Replication Operation = {2}, Last Replication Operation = {3}", queueUsage, queueUsageThreshold, firstLsn, lastLsn);
        writer.WriteLine("Detailed Progress Information:");
        writer.WriteLine("\rActive Configuration Replicas:\r");
        writer.Write(cc);
        writer.WriteLine("\rIdle Replicas:\r");
        writer.Write(idle);
        writer.WriteLine("\rPrevious Configuration Replicas:\r");
        writer.Write(pc);

        return content;
    }

    // 
    // Periodic timer in the replica manager is used for 2 purposes
    // 1. Tracing the PrimaryConfiguration trace even if there is no activity in the system for debugging purposes
    // 2. Increasing the Build complete LSN of the idle replicas
    //
    // Since both of the above operations need to happen periodically based on 2 different config intervals, the timer must run on the lower of the 2 configs
    //
    static TimeSpan GetPeriodicTimerInterval(REInternalSettingsSPtr const & config)
    {
        TimeSpan interval =
            config->TraceInterval > config->IdleReplicaMaxLagDurationBeforePromotion ?
            config->IdleReplicaMaxLagDurationBeforePromotion :
            config->TraceInterval;

        return interval;
    }
};
// End of Static Helper methods

ReplicaManager::ReplicaManager(
    REInternalSettingsSPtr const & config,
    bool hasPersistedState,
    REPerformanceCountersSPtr const & perfCounters,
    FABRIC_REPLICA_ID replicaId,
    bool requireServiceAck,
    ReplicationEndpointId const & endpointUniqueId,
    ReplicationTransportSPtr const & transport,
    FABRIC_SEQUENCE_NUMBER initialProgress,
    FABRIC_EPOCH const & epoch,
    ComProxyStateProvider const & stateProvider,
    ComProxyStatefulServicePartition const & partition,
    IReplicatorHealthClientSPtr const & healthClient,
    ApiMonitoringWrapperSPtr const & apiMonitor,
    Common::Guid const & partitionId)
    :   ComponentRoot(),
        isActive_(false),
        config_(config),
        perfCounters_(perfCounters),
        replicaId_(replicaId),
        endpointUniqueId_(endpointUniqueId),
        transport_(transport), 
        epoch_(epoch),
        stateProvider_(stateProvider),
        partition_(partition),
        partitionId_(partitionId),
        writeQuorum_(1),
        hasEnoughReplicasForQuorum_(true),
        previousWriteQuorum_(0),
        hasEnoughReplicasForQuorumInPreviousConfig_(true),
        activeReplicas_(),
        usePreviousActiveReplicas_(false),
        previousActiveReplicas_(),
        idleReplicas_(), 
        allReplicasApplyAckedLSNinCC_(Constants::InvalidLSN),
        majorityReplicasApplyAckedLSNinCC_(Constants::InvalidLSN),
        replicationQueueManager_(config_, perfCounters_, requireServiceAck, endpointUniqueId_, initialProgress, partitionId_),
        completeReplicateJobQueueUPtr_(nullptr),
        beginReplicateJobQueueUPtr_(nullptr),
        updatingCatchUpReplicaSetConfiguration_(false),
        lock_(),
        waitForPendingOperationsToCompleteEventSPtr_(),
        periodicTimer_(),
        lastQueueFullTraceWatch_(),
        lastTraceWatch_(),
        healthReporter_(),
        apiMonitor_(apiMonitor),
        hasPersistedState_(hasPersistedState)
{
    healthReporter_ = BatchedHealthReporter::Create(
        partitionId_,
        endpointUniqueId_,
        HealthReportType::PrimaryReplicationQueueStatus,
        config_->QueueHealthMonitoringInterval,
        healthClient);
}

ReplicaManager::ReplicaManager(
    REInternalSettingsSPtr const & config,
    bool hasPersistedState,
    REPerformanceCountersSPtr const & perfCounters,
    FABRIC_REPLICA_ID replicaId,
    ReplicationEndpointId const & endpointUniqueId,
    ReplicationTransportSPtr const & transport,
    FABRIC_EPOCH const & epoch,
    ComProxyStateProvider const & stateProvider,
    ComProxyStatefulServicePartition const & partition,
    Common::Guid const & partitionId,
    IReplicatorHealthClientSPtr const & healthClient,
    ApiMonitoringWrapperSPtr const & apiMonitor,
    OperationQueue && queue) 
    :   ComponentRoot(),
        isActive_(false),
        config_(config),
        perfCounters_(perfCounters),
        replicaId_(replicaId),
        endpointUniqueId_(endpointUniqueId),
        transport_(transport), 
        epoch_(epoch),
        stateProvider_(stateProvider),
        partition_(partition),
        partitionId_(partitionId),
        writeQuorum_(0),
        hasEnoughReplicasForQuorum_(false), // Intentionally SET this to FALSE in order to wait for the UpdateCatchupConfiguration Call before processing any potential ACK's
        previousWriteQuorum_(0),
        hasEnoughReplicasForQuorumInPreviousConfig_(false),// Intentionally SET this to FALSE in order to wait for the UpdateCatchupConfiguration Call before processing any potential ACK's
        activeReplicas_(), 
        usePreviousActiveReplicas_(false),
        previousActiveReplicas_(),
        idleReplicas_(), 
        allReplicasApplyAckedLSNinCC_(Constants::InvalidLSN),
        majorityReplicasApplyAckedLSNinCC_(Constants::InvalidLSN),
        replicationQueueManager_(config_, perfCounters_, endpointUniqueId_, partitionId_, move(queue)),
        completeReplicateJobQueueUPtr_(nullptr),
        beginReplicateJobQueueUPtr_(nullptr),
        updatingCatchUpReplicaSetConfiguration_(false),
        lock_(),
        waitForPendingOperationsToCompleteEventSPtr_(),
        periodicTimer_(),
        lastQueueFullTraceWatch_(),
        lastTraceWatch_(),
        idleReplicaBuildCompleteLsnUpdatedWatch_(),
        healthReporter_(),
        apiMonitor_(apiMonitor),
        hasPersistedState_(hasPersistedState)
{
    healthReporter_ = BatchedHealthReporter::Create(
        partitionId_,
        endpointUniqueId_,
        HealthReportType::PrimaryReplicationQueueStatus,
        config_->QueueHealthMonitoringInterval,
        healthClient);
}

ReplicaManager::~ReplicaManager()
{
    AcquireReadLock lock(lock_);
    if (!idleReplicas_.empty() || 
        !activeReplicas_.empty() ||
        !previousActiveReplicas_.empty())
    {
        Assert::CodingError(
            "{0}: ReplicaManager.dtor: {1} idle replicas ({2}), {3} current active ({4}), {5} previous active ({6})", 
            endpointUniqueId_, 
            idleReplicas_.size(),
            Helpers::GetReplicaIdsString(idleReplicas_),
            activeReplicas_.size(),
            Helpers::GetReplicaIdsString(activeReplicas_),
            previousActiveReplicas_.size(),
            Helpers::GetReplicaIdsString(previousActiveReplicas_));
    }
}

OperationQueue && ReplicaManager::get_Queue() 
{ 
    AcquireReadLock lock(lock_);
    ASSERT_IF(
        isActive_,
        "{0}: Can't move the primary queue if replica manager is not closed", 
        endpointUniqueId_);

    return std::move(replicationQueueManager_.Queue); 
}

ReplicationEndpointId ReplicaManager::get_EndpointUniqueId()
{
    return endpointUniqueId_;
}

ErrorCode ReplicaManager::Open()
{
    AcquireWriteLock grab(lock_);

    ErrorCode error = CompleteReplicateJobQueue::Create(
        endpointUniqueId_.ToString(),
        *this,
        static_cast<int>(config_->CompleteReplicateThreadCount),
        completeReplicateJobQueueUPtr_);

    if (!error.IsSuccess())
        return error;

    BeginReplicateJobQueue::Create(
        endpointUniqueId_.ToString(),
        *this,
        0,
        beginReplicateJobQueueUPtr_);

    lastQueueFullTraceWatch_.Start();

    auto root = this->CreateComponentRoot();
    periodicTimer_ = Common::Timer::Create(
        TimerTagDefault,
        [this, root](Common::TimerSPtr const &)
        {
            ReplicationSessionVector nonFaultedIdleReplicas;
            FABRIC_SEQUENCE_NUMBER currentProgress = -1;

            {
                // Trace below under the lock as the tracing method depends on it
                AcquireReadLock acquire(lock_);

                if (!isActive_)
                    return;

                if (lastTraceWatch_.Elapsed > config_->TraceInterval)
                {
                    ReplicatorEventSource::Events->PrimaryConfiguration(
                        partitionId_,
                        endpointUniqueId_,
                        *this,
                        replicationQueueManager_.GetLastSequenceNumber(),
                        config_->ToString());

                    lastTraceWatch_.Restart();
                }

                if (idleReplicaBuildCompleteLsnUpdatedWatch_.Elapsed > config_->IdleReplicaMaxLagDurationBeforePromotion)
                {
                    for (auto const & r : idleReplicas_)
                    {
                        if (!r->IsIdleFaultedDueToSlowProgress)
                        {
                            nonFaultedIdleReplicas.push_back(r);
                        }
                    }

                    if (!nonFaultedIdleReplicas.empty())
                    {
                        // Query progress only if needed
                        currentProgress = replicationQueueManager_.GetCurrentProgress();
                    }

                    idleReplicaBuildCompleteLsnUpdatedWatch_.Restart();
                }

                // Dynamic configs might have changed. Always read from the object
                periodicTimer_->Change(Helpers::GetPeriodicTimerInterval(this->config_));

            }  // Release READ LOCK 

            // After releasing the read lock, do not try to access any more member variables as close could have happened

            for (auto const & r : nonFaultedIdleReplicas)
            {
                r->UpdateLastReplicationSequenceNumberForIdle(currentProgress);
            }
        },
        false);

    periodicTimer_->Change(Helpers::GetPeriodicTimerInterval(config_));
    isActive_ = true;
    lastTraceWatch_.Start();
    idleReplicaBuildCompleteLsnUpdatedWatch_.Start();

    return error;
}

ErrorCode ReplicaManager::Close(bool isCreatingSecondary)
{
    CloseInnerObjects(isCreatingSecondary);
    return ErrorCode(Common::ErrorCodeValue::Success);
}

void ReplicaManager::CloseInnerObjects(bool isCreatingSecondary)
{
    ReplicationSessionVector idleSessions;
    vector<Common::AsyncOperationSPtr> pendingOperations;
    unordered_map<FABRIC_SEQUENCE_NUMBER, AsyncOperationSPtr> replOps;

    {
        AcquireWriteLock lock(lock_);

        if (!isActive_)
            return;

        isActive_ = false;

        periodicTimer_->Cancel();
        periodicTimer_ = nullptr;

        if (!config_->AllowMultipleQuorumSet && updatingCatchUpReplicaSetConfiguration_)
        {
            vector<Common::AsyncOperationSPtr> operationsToComplete;
            updatingCatchUpReplicaSetConfiguration_ = false;
            if (TryGetAndUpdateProgressCallerHoldsLock(operationsToComplete))
            {
                CompleteReplicateOperationsCallerHoldsLock(operationsToComplete);
            }
        }

        // First, take all pending replicate async operations so they are cancelled outside the lock.
        // This has to be done before closing the replicas,
        // otherwise pending replication operations may complete with success 
        // because the primary remains the only replica.
        replOps = move(replicateOperations_);

        // Remove idle sessions
        idleSessions = move(idleReplicas_);
        Helpers::CloseReliableOperationSenders(idleSessions);
    }
        
    // Stop sending replication operations
    beginReplicateJobQueueUPtr_->Close();
    completeReplicateJobQueueUPtr_->Close();

    ReplicatorEventSource::Events->PrimaryReplicateCancel(
        partitionId_,
        endpointUniqueId_, 
        static_cast<uint64>(replOps.size()));

    for (auto const & pair : replOps)
    {
        pair.second->Cancel();
    }
     
    // Cancel any pending build idle operations outside the lock.
    // The primary state machine must make sure 
    // that no new BuildIdle operations are started 
    // after closing was started.
    for (auto const & r : idleSessions)
    {
        r->CancelCopy();
    }

    {
        AcquireWriteLock lock(lock_);
            
        // Stop replication sesion operations holder,
        // to make sure operations are not sent to the remote replicas
        Helpers::CloseReliableOperationSenders(activeReplicas_);
        Helpers::CloseReliableOperationSenders(previousActiveReplicas_);
        healthReporter_->Close();

        if (isCreatingSecondary)
            UpdateProgressPrivateCallerHoldsLock(true);
        
        auto activeSessions = move(activeReplicas_);
        auto previousActiveSessions = move(previousActiveReplicas_);
        auto mover = move(healthReporter_);
    }
}

ErrorCode ReplicaManager::UpdateEpochAndGetLastSequenceNumber(
    FABRIC_EPOCH const & epoch,
    FABRIC_SEQUENCE_NUMBER & lastSequenceNumber)
{
    ErrorCode error;
    lastSequenceNumber = Constants::NonInitializedLSN;
    
    {
        AcquireWriteLock lock(lock_);
        
        if (epoch < epoch_)
        {
            ReplicatorEventSource::Events->PrimaryUpdateEpoch(
                partitionId_, 
                endpointUniqueId_,
                epoch_.DataLossNumber,
                epoch_.ConfigurationNumber,
                epoch.DataLossNumber,
                epoch.ConfigurationNumber,
                L"ERROR: new epoch smaller than current one");

            error = ErrorCode(Common::ErrorCodeValue::REInvalidEpoch);
        }
        else if (epoch == epoch_)
        {
            // Duplicate call, just return last sequence number
            lastSequenceNumber = replicationQueueManager_.GetLastSequenceNumber();
        }
        else
        {
            ReplicatorEventSource::Events->PrimaryUpdateEpoch(
                partitionId_, 
                endpointUniqueId_,
                epoch_.DataLossNumber,
                epoch_.ConfigurationNumber,
                epoch.DataLossNumber,
                epoch.ConfigurationNumber,
                L"OK");

            epoch_ = epoch;
            lastSequenceNumber = replicationQueueManager_.GetLastSequenceNumber();
        }

        // Notify all sessions that the epoch has changed. Duplicate calls are okay
        for(auto session : activeReplicas_)
            session->OnUpdateEpoch(epoch);
        for(auto session : previousActiveReplicas_)
            session->OnUpdateEpoch(epoch);
        for(auto session : idleReplicas_)
            session->OnUpdateEpoch(epoch);
    }

    return error;
}

AsyncOperationSPtr ReplicaManager::BeginWaitForReplicateOperationsCallbacksToComplete(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & state)
{
    {
        AcquireReadLock lock(lock_);

        // The primary state machine will ensure that this method is not called if the replica manager is not closed
        ASSERT_IF(
            isActive_,
            "{0}: Cannot wait for replication operations to complete when replicamanager state is",
            endpointUniqueId_);
    }
    
    return completeReplicateJobQueueUPtr_->OperationsFinishedAsyncEvent.BeginWaitOne(
        TimeSpan::MaxValue,
        callback,
        state);
}

ErrorCode ReplicaManager::EndWaitForReplicateOperationsCallbacksToComplete(
    AsyncOperationSPtr const & asyncOperation)
{
    return completeReplicateJobQueueUPtr_->OperationsFinishedAsyncEvent.EndWaitOne(asyncOperation);
}

AsyncOperationSPtr ReplicaManager::BeginWaitForReplicateOperationsToComplete(
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & state)
{
    ASSERT_IF(timeout == TimeSpan::Zero, "{0}: ReplicaManager::BeginWaitForReplicateOperationsToComplete expects timeout period > 0", endpointUniqueId_);
    bool shouldWait = false;
    ErrorCode error;
    std::shared_ptr<AsyncManualResetEvent> waitEventCopy;

    {
        AcquireWriteLock grab(lock_);
        
        if (replicateOperations_.size() > 0)
        {
            waitEventCopy = std::make_shared<AsyncManualResetEvent>();
            shouldWait = true; 

            ASSERT_IF(waitForPendingOperationsToCompleteEventSPtr_, "{0}: ReplicaManager::BeginWaitForReplicateOperationsToComplete cannot have 2 wait events", endpointUniqueId_);
            waitForPendingOperationsToCompleteEventSPtr_ = waitEventCopy;
        }
    }

    if (shouldWait)
    {
        return waitEventCopy->BeginWaitOne(timeout, callback, state);
    }
    else
    {
        return CompletedAsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }
}
            
ErrorCode ReplicaManager::EndWaitForReplicateOperationsToComplete(
    AsyncOperationSPtr const & asyncOperation)
{
    ErrorCode error;
    auto asyncOp = dynamic_cast<CompletedAsyncOperation*>(asyncOperation.get());

    if (asyncOp)
    {
        error = CompletedAsyncOperation::End(asyncOperation);
    }
    else
    {
        std::shared_ptr<AsyncManualResetEvent> waitEventCopy;
        {
            AcquireWriteLock grab(lock_);
            waitEventCopy = move(waitForPendingOperationsToCompleteEventSPtr_);
        }

        ASSERT_IFNOT(waitEventCopy, "{0}: ReplicaManager::EndWaitForReplicateOperationsToCompelete - pending async await handle cannot be empty", endpointUniqueId_);

        error = waitEventCopy->EndWaitOne(asyncOperation);
    }

    return error;
}


// *****************************
// Replication queue related methods
// *****************************
ErrorCode ReplicaManager::ResetReplicationQueue(FABRIC_SEQUENCE_NUMBER newProgress)
{
    AcquireWriteLock lock(lock_);
    if (!isActive_)
        return ErrorCode(Common::ErrorCodeValue::ObjectClosed);

    replicationQueueManager_.ResetQueue(newProgress);
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode ReplicaManager::GetCurrentProgress(
    __out FABRIC_SEQUENCE_NUMBER & lastSequenceNumber) const
{
    AcquireReadLock lock(lock_);
    if (!isActive_)
        return ErrorCode(Common::ErrorCodeValue::ObjectClosed);

    lastSequenceNumber = replicationQueueManager_.GetCurrentProgress();

    return {};
}

ErrorCode ReplicaManager::GetCatchUpCapability(
    __out FABRIC_SEQUENCE_NUMBER & firstSequenceNumber) const
{
    AcquireReadLock lock(lock_);
    if (!isActive_)
        return ErrorCode(Common::ErrorCodeValue::ObjectClosed);

    firstSequenceNumber = replicationQueueManager_.GetCatchUpCapability();

    return {};
}

ErrorCode ReplicaManager::GetLastSequenceNumber(
    __out FABRIC_SEQUENCE_NUMBER & sequenceNumber) const
{
    AcquireReadLock lock(lock_);
    if (!isActive_)
        return ErrorCode(Common::ErrorCodeValue::ObjectClosed);

    sequenceNumber = replicationQueueManager_.GetLastSequenceNumber();

    return {};
}

bool ReplicaManager::GetQueueProgressForCatchup(
    __out bool & hasEnoughReplicasForQuorum,
    __out FABRIC_SEQUENCE_NUMBER & quorumAckLSN,
    __out FABRIC_SEQUENCE_NUMBER & allAckLSN,
    __out FABRIC_SEQUENCE_NUMBER & latestLSN,
    __out FABRIC_SEQUENCE_NUMBER & previousConfigCatchupLsn,
    __out FABRIC_SEQUENCE_NUMBER & lowestLSNAmongstMustCatchupReplicas) const
{
    AcquireReadLock lock(lock_);
    if (!isActive_)
    {
        ReplicatorEventSource::Events->PrimaryReplMgrClosed(
            partitionId_,
            endpointUniqueId_,
            L"GetQueueProgressForCatchup");
        return false;
    }

    hasEnoughReplicasForQuorum = hasEnoughReplicasForQuorum_;
    lowestLSNAmongstMustCatchupReplicas = Constants::MaxLSN;

    if (hasEnoughReplicasForQuorum_)
    {
        replicationQueueManager_.GetQueueProgressForCatchup(
            majorityReplicasApplyAckedLSNinCC_,
            allReplicasApplyAckedLSNinCC_,
            quorumAckLSN,
            allAckLSN,
            latestLSN,
            previousConfigCatchupLsn);

        for(auto const & activeReplica : activeReplicas_)
        {
            auto lastAckedLsn = activeReplica->GetLastApplyLsn(replicationQueueManager_.FirstLSNInReplicationQueue - 1);
            if (activeReplica->MustCatchup == MustCatchup::Enum::Yes &&
                lastAckedLsn < lowestLSNAmongstMustCatchupReplicas)
            {
                lowestLSNAmongstMustCatchupReplicas = lastAckedLsn;
            }
        }
    }

    return true;
}

ErrorCode ReplicaManager::GetReplicationQueueCounters(
    __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters)
{
    AcquireReadLock grab(lock_);
    if (!isActive_)
        return Common::ErrorCodeValue::ObjectClosed;

    return replicationQueueManager_.GetReplicationQueueCounters(counters);
}

ErrorCode ReplicaManager::GetReplicatorStatus(
    __out ServiceModel::ReplicatorStatusQueryResultSPtr & result)
{
    AcquireReadLock grab(lock_);

    ReplicationSessionVector mergedPCandCC = activeReplicas_;
    for (auto it = previousActiveReplicas_.begin(); it != previousActiveReplicas_.end(); it++)
    {
        auto replicaFound = std::find_if(mergedPCandCC.begin(), mergedPCandCC.end(),
            [&] (ReplicationSessionSPtr const & r)->bool 
            { 
                return r->ReplicaId == (*it)->ReplicaId;
            });

        if (replicaFound == mergedPCandCC.end())
        {
            mergedPCandCC.push_back((*it));
        }
    }

    return replicationQueueManager_.GetReplicatorStatus(idleReplicas_, mergedPCandCC, result);
}
   
// *****************************
// Replication sessions related methods
// *****************************
ErrorCode ReplicaManager::TryAddIdleReplica(
    ReplicaInformation const & replica,
    __out ReplicationSessionSPtr & session,
    __out FABRIC_SEQUENCE_NUMBER & replicationStartSeq)
{
    FABRIC_SEQUENCE_NUMBER currentCompletedLsn = Constants::InvalidLSN;
    ComOperationRawPtrVector pendingOperations;
    ASSERT_IFNOT(
        replica.Role == FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
        "{0}: AddIdle: can't add active replica {1}",
        endpointUniqueId_,
        replica.Id);
    
    {
        AcquireWriteLock lock(lock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->PrimaryReplMgrClosed(
                partitionId_,
                endpointUniqueId_,
                L"AddIdle");
            return ErrorCode(Common::ErrorCodeValue::ObjectClosed);
        }
        
        ReplicationSessionSPtr temp;
        if (FindReplicaCallerHoldsLock(replica, temp))
        {
            // The replica already exists, can't be added
            ReplicatorEventSource::Events->PrimaryAddIdleError(
                partitionId_, 
                endpointUniqueId_, 
                replica.Id);
            return ErrorCode(Common::ErrorCodeValue::REReplicaAlreadyExists);
        }

        // No progress should be specified for the idle replica
        ASSERT_IF(
            replica.CurrentProgress > Constants::InvalidLSN,
            "{0}: Idle replica shouldn't provide any progress. Instead, current progress is {1}",
            endpointUniqueId_,
            replica.CurrentProgress);
       
        session = CreateReplica(replica);
        idleReplicas_.push_back(session);

        // Get replication start sequence (the committed LSN + 1)
        // and the pending operations
        ASSERT_IFNOT(
            replicationQueueManager_.GetPendingOperationsToSendDuringBuild(stateProvider_.SupportsCopyUntilLatestLsn, replicationStartSeq, pendingOperations),
            "{0}: Missing operations while adding them to the idle replica {1}",
            endpointUniqueId_,
            session->ReplicaId);

        currentCompletedLsn = replicationQueueManager_.FirstLSNInReplicationQueue - 1;
    }

    // Send the pending replicate operations that are not yet quorumed ACK
    // outside the lock
    session->AddReplicateOperations(pendingOperations, currentCompletedLsn);
    
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ReplicationSessionSPtr ReplicaManager::CreateReplica(
    ReplicaInformation const & replica)
{
    ASSERT_IF(replica.Role == FABRIC_REPLICA_ROLE_PRIMARY, 
        "{0}: Can't add another primary replica.", endpointUniqueId_);
    ASSERT_IF(replica.Id == replicaId_, 
        "{0}: Can't add the local replica {1} to the list of replicas.", endpointUniqueId_, replica);

    ReplicationSessionSPtr session = std::make_shared<ReplicationSession>(
        config_, partition_, replica.Id, replica.ReplicatorAddress, replica.TransportEndpointId, replica.CurrentProgress, endpointUniqueId_, partitionId_, epoch_, apiMonitor_, transport_);
    session->Open();
    
    return session;
}

bool ReplicaManager::GetPendingOperationsCallerHoldsLock(
    ReplicationSessionSPtr const & session,
    __out ReplicationSessionPendingOperationsMap & secondariesWithPendingOps)
{
    FABRIC_SEQUENCE_NUMBER last = session->GetLastApplyLsn(replicationQueueManager_.FirstLSNInReplicationQueue - 1);
    // Check whether the replica needs to be caught up - 
    // give it all pending operations starting with LastAckedLSN + 1
    ComOperationRawPtrVector pendingOperations;

    bool pendingOperationsExist = replicationQueueManager_.GetOperations(last + 1, pendingOperations);

    if (pendingOperationsExist)
    {
        if (!pendingOperations.empty())
        {
            secondariesWithPendingOps.insert(ReplicationSessionPendingOperationsEntry(
                session,
                move(pendingOperations)));
        }
    }

    return pendingOperationsExist;
}

ErrorCode ReplicaManager::UpdateConfiguration(
    ReplicaInformationVector const & previousReplicas, 
    ULONG previousWriteQuorum,
    ReplicaInformationVector const & currentReplicas, 
    ULONG currentWriteQuorum,
    bool updatingCatchUpReplicaSetConfiguration,
    bool testOnly)
{
    ReplicationSessionPendingOperationsMap secondariesWithPendingOps;
    vector<Common::AsyncOperationSPtr> operationsToComplete;
    FABRIC_SEQUENCE_NUMBER currentCompletedLsn = Constants::InvalidLSN;

    {
        AcquireWriteLock lock(lock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->PrimaryReplMgrClosed(
                partitionId_,
                endpointUniqueId_,
                L"TryUpdateConfiguration");
            return Common::ErrorCodeValue::ObjectClosed;
        }

        if (!ValidateReplicas(currentReplicas))
        {
            return Common::ErrorCodeValue::OperationFailed;
        }

        TryUpdateReplicasCallerHoldsLock(
            previousWriteQuorum,
            previousReplicas,
            currentReplicas,
            secondariesWithPendingOps);

        updatingCatchUpReplicaSetConfiguration_ = updatingCatchUpReplicaSetConfiguration;

        for(auto const & pcReplica : previousReplicas)
        {
            ASSERT_IF(pcReplica.MustCatchup, "{0}: Mustcatchup flag not expected to be true for PC set during UpdateConfiguration for replica {1}", endpointUniqueId_, pcReplica.Id);
        }

        if (updatingCatchUpReplicaSetConfiguration_)
        {
            for(auto const & currentReplica : currentReplicas)
            {
                ReplicationSessionSPtr session;
                ASSERT_IFNOT(FindReplicaCallerHoldsLock(currentReplica, session), "{0}: Could not find replica {1} in active list", endpointUniqueId_, currentReplica.Id);

                if (!AreEqual(session->MustCatchup, currentReplica.MustCatchup))
                {
                    session->MustCatchup =
                        currentReplica.MustCatchup == true ?
                        MustCatchup::Enum::Yes :
                        MustCatchup::Enum::No;
                }
            }
        }
        else
        {
            for(auto const & currentReplica : currentReplicas)
            {
                ASSERT_IF(currentReplica.MustCatchup, "{0}: Mustcatchup flag not expected to be true during UpdateCurrentConfiguration for replica {1}", endpointUniqueId_, currentReplica.Id);
            }

            for(auto const & activeReplica : activeReplicas_)
            {
                activeReplica->MustCatchup = MustCatchup::Unknown;
            }
        }

        replicationQueueManager_.UpdateCatchupCompletionLSN();

        writeQuorum_ = currentWriteQuorum;
        previousWriteQuorum_ = previousWriteQuorum;

        hasEnoughReplicasForQuorum_ = HasEnoughReplicas(activeReplicas_, currentWriteQuorum);
        hasEnoughReplicasForQuorumInPreviousConfig_ = HasEnoughReplicas(previousActiveReplicas_, previousWriteQuorum);
        
        if (testOnly)
        {
            return Common::ErrorCodeValue::Success;
        }

        if (TryGetAndUpdateProgressCallerHoldsLock(operationsToComplete))
        {
            // Complete the replicate async operations for the committed LSNs
            CompleteReplicateOperationsCallerHoldsLock(operationsToComplete);
        }

        currentCompletedLsn = replicationQueueManager_.FirstLSNInReplicationQueue - 1;
    }

    // Add pending operations to each session outside the lock,
    // because it involves sending operations over the wire
    for (auto const & secondary : secondariesWithPendingOps)
    {
        ReplicatorEventSource::Events->PrimaryAddSessionInitialRepl(
            this->partitionId_,
            this->endpointUniqueId_,
            (secondary.first)->ReplicaId,
            static_cast<uint64>(secondary.second.size()));
        (secondary.first)->AddReplicateOperations(secondary.second, currentCompletedLsn);
    }
       
    return Common::ErrorCodeValue::Success;
}

bool ReplicaManager::HasEnoughReplicas(
    ReplicationSessionVector const & replicas, 
    ULONG writeQuorum)
{
    if (writeQuorum > 1)
    {
        uint64 activeReplicaQuorum = static_cast<uint64>(writeQuorum - 1);
        uint64 activeReplicaCount = static_cast<uint64>(replicas.size());
        // Check whether there are enough replicas for quorum in current configuration
        if (activeReplicaQuorum > activeReplicaCount)
        {
            // There are not enough replicas
            ReplicatorEventSource::Events->PrimaryMissingReplicas(
                partitionId_,
                endpointUniqueId_,
                activeReplicaCount,
                activeReplicaQuorum);

            return false;
        }
    }

    return true;
}

bool ReplicaManager::ValidateReplicas(
    ReplicaInformationVector const & currentReplicas)
{
    // Validate active replica list against current active and idle list
    for (auto it = currentReplicas.begin(); it != currentReplicas.end(); ++it)
    {
        FABRIC_REPLICA_ID replicaId = it->Id;
            
        auto itActive = std::find_if(activeReplicas_.begin(), activeReplicas_.end(), 
        [replicaId] (ReplicationSessionSPtr const & r)->bool { 
            return r->ReplicaId == replicaId; });

        if (itActive != activeReplicas_.end() || it->CurrentProgress != FABRIC_INVALID_SEQUENCE_NUMBER)
        {
            // Replica is already in the active list or the progress information is not known by the system
            continue;
        }

        // Check if this replica is in idle list to be promoted
        auto itIdle = std::find_if(idleReplicas_.begin(), idleReplicas_.end(), 
        [replicaId] (ReplicationSessionSPtr const & r)->bool { 
            return r->ReplicaId == replicaId; });

        if (itIdle == idleReplicas_.end())
        {
            // Could not find this replica in the idle list
            // ignore update
            ReplicatorEventSource::Events->PrimaryIdleDoesNotExist(
                partitionId_,
                endpointUniqueId_,
                replicaId);
            return false;
        }
    }

    return true;
}

bool ReplicaManager::TryUpdateReplicasCallerHoldsLock(
    ULONG previousWriteQuorum,
    ReplicaInformationVector const & previousReplicas, 
    ReplicaInformationVector const & currentReplicas, 
    __out ReplicationSessionPendingOperationsMap & secondariesWithPendingOps)
{
    ReplicationSessionVector possibleReplicasToClose;
    bool changeCurrentReplicas = false;

    // Keep track of the processed replicas to identify 
    // replicas that must be added to the current/previous configuration.
    vector<bool> processedReplicas;
    processedReplicas.resize(currentReplicas.size(), false);
    vector<bool> processedPreviousReplicas;
    processedPreviousReplicas.resize(previousReplicas.size(), false);
    
    if (!usePreviousActiveReplicas_ && !previousActiveReplicas_.empty())
    {
        Assert::CodingError(
            "{0}: use previous replicas deactivated but previous replicas not empty {1}",
            endpointUniqueId_,
            Helpers::GetReplicaIdsString(previousActiveReplicas_));
    }

    bool previousReplicasDone;
    // Keep track of old previous active replicas;
    // the ones that are not in the new vectors must be closed.
    ReplicationSessionVector oldPreviousActiveReplicas = move(previousActiveReplicas_);
    
    if (previousWriteQuorum == 0)
    {
        // Deactivate previous config
        ASSERT_IFNOT(
            previousReplicas.empty(),
            "{0}: previous quorum is 0, but the previous config contains {1} replicas",
            endpointUniqueId_,
            previousReplicas.size());
        usePreviousActiveReplicas_ = false;
        previousReplicasDone = true;
    }
    else
    {
        usePreviousActiveReplicas_ = true;

        if (!oldPreviousActiveReplicas.empty())
        {
            // The new previous config must be a subset of the old previous config.
            // The rest must be closed if they are neither in the current config nor idle list
            for (auto it = oldPreviousActiveReplicas.begin(); it != oldPreviousActiveReplicas.end(); ++it)
            {
                if (Helpers::GetReplicaIter(previousReplicas, *it) != previousReplicas.end())
                {
                    previousActiveReplicas_.push_back(move(*it));
                }
                else
                {
                    changeCurrentReplicas = true;
                }
            }

            previousReplicasDone = true;
        }
        else
        {
            previousReplicasDone = previousReplicas.empty();
        }
    }

    // Look in the current list of active replicas 
    // and match with the new changes:
    // If the replica exists in the new configuration, 
    // update it; otherwise, don't move it to the new vector.
    if (!activeReplicas_.empty())
    {
        ReplicationSessionVector oldActiveReplicas = move(activeReplicas_);
        for (auto it = oldActiveReplicas.begin(); it != oldActiveReplicas.end(); ++it)
        {
            bool change = ProcessActiveReplicaCallerHoldsLock(
                previousReplicas,
                currentReplicas, 
                *it,
                previousReplicasDone,
                processedReplicas,
                processedPreviousReplicas, 
                possibleReplicasToClose);
            changeCurrentReplicas = changeCurrentReplicas || change;
        }

        previousReplicasDone = true;
        oldActiveReplicas.clear();
    }

    if (!idleReplicas_.empty())
    {
        // Check whether any of the idle replicas is promoted to secondary
        ReplicationSessionVector oldIdleReplicas = move(idleReplicas_);
        for (auto it = oldIdleReplicas.begin(); it != oldIdleReplicas.end(); ++it)
        {
            bool change = ProcessIdleReplicaCallerHoldsLock(
                currentReplicas, 
                previousReplicas,
                *it,
                processedReplicas,
                processedPreviousReplicas);
            changeCurrentReplicas = changeCurrentReplicas || change;
        }
        oldIdleReplicas.clear();
    }

    for (auto itOldPrevious = oldPreviousActiveReplicas.begin(); itOldPrevious != oldPreviousActiveReplicas.end(); ++itOldPrevious)
    {
        if (*itOldPrevious)
        {
            // check if any of the replicas in the old PC that are NOT in the current PC need to either close or moved to cc
            bool change = ProcessActiveReplicaCallerHoldsLock(
                previousReplicas,
                currentReplicas, 
                *itOldPrevious,
                previousReplicasDone,
                processedReplicas,
                processedPreviousReplicas, 
                possibleReplicasToClose);
            changeCurrentReplicas = changeCurrentReplicas || change;
        }
    }

    // The current replicas that were not processed yet
    // must be added.
    for (size_t i = 0; i < processedReplicas.size(); ++i)
    {
        if (!processedReplicas[i])
        {
            // The new replica should have a valid progress specified
            ReplicationSessionSPtr const & newSecondary = CreateReplica(currentReplicas[i]);

            ASSERT_IFNOT(
                GetPendingOperationsCallerHoldsLock(newSecondary, secondariesWithPendingOps),
                "{0}: Primary missing operations that need to be sent to Replica {1} in CC",
                endpointUniqueId_, newSecondary->ReplicaId);

            changeCurrentReplicas = true;
            if (!previousReplicasDone)
            {
                // If the replica is also in the previous config replicas,
                // add it to the previous active sessions
                auto it = Helpers::GetReplicaIter(previousReplicas, newSecondary);
                if (it != previousReplicas.end())
                {
                    previousActiveReplicas_.push_back(newSecondary);
                    processedPreviousReplicas[it - previousReplicas.begin()] = true;
                }
            }

            activeReplicas_.push_back(move(newSecondary));
        }
    }

    // Add any remaining sessions to previous active configuration
    if (!previousReplicasDone)
    {
        for (size_t i = 0; i < processedPreviousReplicas.size(); ++i)
        {
            if (!processedPreviousReplicas[i])
            {
                // The new replica should have a valid progress specified
                ReplicationSessionSPtr const & newSecondary = CreateReplica(previousReplicas[i]);
                if (!GetPendingOperationsCallerHoldsLock(newSecondary, secondariesWithPendingOps))
                {
                    ReplicatorEventSource::Events->PrimaryMissingOperations(
                        partitionId_,
                        endpointUniqueId_,
                        newSecondary->ReplicaId);

                    // If pending operations are missing, we move this session to a paused state by closing the reliable operation senders
                    // 
                    // It is possible to run into this situation where the primary does not
                    // have operations to send to a slow replica in PC (especially if this primary was just promoted from active secondary and 
                    // the config->SecondaryClearAcknowledgedOperations was set to TRUE)
                    //
                    // It is okay to keep this replica in a paused state and not ASSERT as PC replicas are not considered for quorum during
                    // catchup.
                    newSecondary->Close();
                }

                previousActiveReplicas_.push_back(move(newSecondary));
                changeCurrentReplicas = true;
            }
        }
    }

    if (changeCurrentReplicas)
    {
        ReplicatorEventSource::Events->PrimaryConfiguration(
            partitionId_,
            endpointUniqueId_,
            *this,
            replicationQueueManager_.GetLastSequenceNumber(),
            config_->ToString());
    }

    if (previousReplicas.size() != previousActiveReplicas_.size())
    {
        Assert::CodingError(
            "{0}: UpdateConfig: processing previous active replicas error: previousReplicas={1}; previousActiveReplicas_={2}",
            endpointUniqueId_,
            previousReplicas.size(),
            Helpers::GetReplicaIdsString(previousActiveReplicas_));
    }

    if (currentReplicas.size() != activeReplicas_.size())
    {
        Assert::CodingError(
            "{0}: UpdateConfig: processing active replicas error: currentReplicas={1}; activeReplicas_={2}",
            endpointUniqueId_,
            currentReplicas.size(),
            Helpers::GetReplicaIdsString(activeReplicas_));
    }

    // Close the replication sessions operation holder
    // for the replicas that are no more in active or previous active config.
    // This will cancel the timer and stop sending
    // operations to the remote replicas.
    // Close sessions is done inside the lock;
    // this should be safe, since the close code just cancels the timers
    // and the sender never calls inside ReplicationSession 
    // while holding its own lock.
    if (!possibleReplicasToClose.empty())
    {
        ReplicationSessionVector replicasToClose;
        for (auto itCloseReplica = possibleReplicasToClose.begin(); itCloseReplica != possibleReplicasToClose.end(); ++itCloseReplica)
        {
            auto isPresentInIdle = Helpers::GetReplicaFromAddress(
                idleReplicas_,
                (*itCloseReplica)->ReplicationEndpoint,
                (*itCloseReplica)->EndpointUniqueId);

            // We don't close replicas which are still present in Idle list
            if (!isPresentInIdle)
            {
                ReplicatorEventSource::Events->PrimaryRemoveReplica(
                    partitionId_, 
                    endpointUniqueId_, 
                    (*itCloseReplica)->ReplicaId);

                replicasToClose.push_back(*itCloseReplica);
            }
        }

        if (!replicasToClose.empty())
        {
            Helpers::CloseReliableOperationSenders(replicasToClose);
        }
    }
    
    return changeCurrentReplicas;
}

bool ReplicaManager::ProcessActiveReplicaCallerHoldsLock(
    ReplicaInformationVector const & previousReplicas, 
    ReplicaInformationVector const & currentReplicas, 
    ReplicationSessionSPtr const & session,
    bool previousReplicasDone,
    __inout vector<bool> & processedReplicas,
    __inout vector<bool> & processedPreviousReplicas,
    __inout ReplicationSessionVector & possibleReplicasToClose)
{
    bool closeReplica = true;
    bool changeCurrentReplicas = false;
    
    if (!previousReplicasDone)
    {
        // Check whether replica is in the previous config vector
        auto it = Helpers::GetReplicaIter(previousReplicas, session);
        if (it != previousReplicas.end())
        {
            // Make a copy of the replica and place it in the previous config vector
            previousActiveReplicas_.push_back(session);
            processedPreviousReplicas[it - previousReplicas.begin()] = true;
            // The replica is still used, so there is no need to close it
            closeReplica = false;
        }
    }

    auto it = Helpers::GetReplicaIter(currentReplicas, session);
    if (it == currentReplicas.end())
    {
        changeCurrentReplicas = true;
    }
    else
    {
        if (!processedReplicas[it - currentReplicas.begin()])
        {
            processedReplicas[it - currentReplicas.begin()] = true;
            // Update the replica to the new parameters
            ASSERT_IF(
                it->Role != FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY,
                "{0}: Can't update replica {1} from secondary to idle or Primary", 
                endpointUniqueId_, 
                session);
            activeReplicas_.push_back(move(session));
        }

        // The replica is still used, so there is no need to close it
        closeReplica = false;
    }

    if (closeReplica)
    {
        possibleReplicasToClose.push_back(move(session));
    }

    return changeCurrentReplicas;
}

bool ReplicaManager::ProcessIdleReplicaCallerHoldsLock(
    ReplicaInformationVector const & currentReplicas,
    ReplicaInformationVector const & previousReplicas,
    ReplicationSessionSPtr const & session,
    __inout vector<bool> & processedReplicas,
    __inout vector<bool> & processedPreviousReplicas)
{
    bool pcChanged = false;
    bool ccChanged = false;

    auto it = Helpers::GetReplicaIter(currentReplicas, session);
    if (it != currentReplicas.end())
    {
        processedReplicas[it - currentReplicas.begin()] = true;
        // Promote from idle to secondary
        ReplicatorEventSource::Events->PrimaryPromoteIdle(
            partitionId_, 
            endpointUniqueId_, 
            it->Id);

        ASSERT_IF(session->IsIdleFaultedDueToSlowProgress == true, "Replica {0} cannot be promoted to secondary as it is faulted", session);
        ASSERT_IF(it->Role != FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, "Idle replica {0} can only be promoted to secondary", session);
        session->OnPromoteToActiveSecondary();
        activeReplicas_.push_back(session);
        ccChanged = true;
    }

    it = Helpers::GetReplicaIter(previousReplicas, session);
    if (it != previousReplicas.end())
    {
        processedPreviousReplicas[it - previousReplicas.begin()] = true;

        // This replica could already be in the previousActiveReplicas_ list if replicator receives a duplicate UC call
        // We will only add if its not already present
        auto existing = Helpers::GetReplicaFromAddress(
            previousActiveReplicas_,
            session->ReplicationEndpoint,
            session->EndpointUniqueId);
        
        if (!existing)
        {
            previousActiveReplicas_.push_back(session);
            pcChanged = true;
        }
    }

    if (!ccChanged)
    {
        // No change in CC, keep the replica in idle list
        idleReplicas_.push_back(move(session));
    }

    return ccChanged || pcChanged;
}

bool ReplicaManager::RemoveReplica(FABRIC_REPLICA_ID replicaId)
{
    ReplicationSessionSPtr session;
    vector<Common::AsyncOperationSPtr> operationsToComplete;
    ReplicatorEventSource::Events->PrimaryRemoveReplica(
        partitionId_, 
        endpointUniqueId_, 
        replicaId);
            
    {
        AcquireWriteLock lock(lock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->PrimaryReplMgrClosed(
                partitionId_,
                endpointUniqueId_,
                L"RemoveReplica");
            return false;
        }

        auto itIdle = std::find_if(idleReplicas_.begin(), idleReplicas_.end(), 
            [replicaId] (ReplicationSessionSPtr const & r)->bool { 
                return r->ReplicaId == replicaId; });

        if (itIdle != idleReplicas_.end())
        {
            std::swap(session, *itIdle);
            idleReplicas_.erase(itIdle);

            // Close the replications session holder 
            // that retries sending operations
            session->Close();

            if (TryGetAndUpdateProgressCallerHoldsLock(operationsToComplete))
            {
                // Complete the replicate async operations for the committed operations (if any)
                CompleteReplicateOperationsCallerHoldsLock(operationsToComplete);
            }
        }
    }

    if (session == nullptr)
    {
        return false;
    }
    else
    {
        session->CancelCopy();
        return true;
    }
}

ErrorCode ReplicaManager::AddReplicateOperation(
    AsyncOperationSPtr const & thisSPtr,
    ComPointer<IFabricOperationData> && comOperationPointer,
    __out bool & operationCommitted,
    __out ComOperationCPtr & opComPtr)
{
    uint replicationQueueUsagePercent = 0;
    
    auto error = AddReplicateOperationPrivate(
        thisSPtr,
        move(comOperationPointer),
        replicationQueueUsagePercent,
        operationCommitted,
        opComPtr);

    if (replicationQueueUsagePercent >= config_->SlowIdleRestartAtQueueUsagePercent &&
        Helpers::RestartSlowIdleConfigEnabled(hasPersistedState_, config_)) // Always use the config object to check for this setting as its a dynamic setting
    {
        FaultSlowIdleReplicasIfPossible(replicationQueueUsagePercent);
    }

    if (replicationQueueUsagePercent >= config_->SlowActiveSecondaryRestartAtQueueUsagePercent &&
       Helpers::RestartSlowActiveSecondaryConfigEnabled(hasPersistedState_, config_)) // Always use the config object to check for this setting as its a dynamic setting
    {
        FaultSlowActiveSecondaryReplicasIfPossible(replicationQueueUsagePercent);
    }
    
    return error;
}
            

// *******************************************NOTE - FAULTING SLOW REPLICAS***************************************************
//  Implementation spec of the below method is here - 
//  https://microsoft.sharepoint.com/teams/WindowsFabric/_layouts/OneNote.aspx?id=%2Fteams%2FWindowsFabric%2FNotebooks%2FV2%2FFailover%2FFailover&wd=target%28V1Replicator.one%7C386E56AB-8E09-46E9-A94E-4A2AC457BA81%2FSlow%20Active%20Secondary%20Detection%20and%20Mitigation%7C5E48B351-015A-4EB1-AD1B-E001395F9C28%2F%29
//  onenote:https://microsoft.sharepoint.com/teams/WindowsFabric/Notebooks/V2/Failover/Failover/V1Replicator.one#Slow%20Active%20Secondary%20Detection%20and%20Mitigation&section-id={386E56AB-8E09-46E9-A94E-4A2AC457BA81}&page-id={5E48B351-015A-4EB1-AD1B-E001395F9C28}&end
// ***************************************************************************************************************************
void ReplicaManager::FaultSlowActiveSecondaryReplicasIfPossible(uint replicationQueueUsagePercent)
{
    ReplicationSessionSPtr toBeRestartedReplica = nullptr;
    StandardDeviation avgReceiveAckDuration, avgApplyAckDuration;
    FABRIC_SEQUENCE_NUMBER firstLsn, lastLsn;

    {
        AcquireReadLock grab(lock_);

        // CHECK 1 - We are not under quorum loss or write quorum != 1
        if (!hasEnoughReplicasForQuorum_ ||
            writeQuorum_ <= 1)
        {
            return;
        }

        ReplicationSessionVector nonFaultedActiveReplicas = Helpers::GetActiveReplicasThatAreNotFaultedDueToSlowProgress(activeReplicas_);

        // CHECK 2 - Ensure taking down a secondary will NOT result in WQL
        if (nonFaultedActiveReplicas.size() < writeQuorum_ + static_cast<uint>(config_->ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness)) // use config object as it is a dynamic config
        {
            // If taking down an active replica will cause write quorum loss, return
            // Also, return if user has configured a min number of replicas greater than write quorum to be in the set even though they are slow
            // 
            // write quorum includes Primary and hence the actual formula for the above is:
            //  var newActiveCountAfterTakingDownSecondary = activeReplicas_.size() - 1;
            //  var includePrimaryCount = 1;
            //  
            //      if (newActiveCountAfterTakingDownSecondary + includePrimaryCount < writeQuorum_ + config)
            //          return;
            //
            // The above is simplied by cancelling the +1 and -1 on LHS

            return;
        }

        // CHECK 3 - Protection against burst of incoming requests that just arrived
        if (replicationQueueManager_.OperationCount == 0 ||
            replicationQueueManager_.FirstOperationInReplicationQueueEnqueuedSince < config_->SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation)
        {
            // check directly on config object as it can change underneath since it is a dynamic config
            return;
        }

        // CHECK 4 - Protection against ALL or more than a write quorum of replicas being slow
        if (majorityReplicasApplyAckedLSNinCC_ <= replicationQueueManager_.FirstLSNInReplicationQueue)
        {
            // If the quorum lsn of CC is less than or equal to first LSN in replication queue, it implies that none of the operations in the queue have been quorum ACK'd
            // This indicates all or a majority quorum of replicas are slow and hence there is no advantage of restarting any secondary
            return;
        }

        toBeRestartedReplica = Helpers::GetSlowestReplicaIfPresent(
            nonFaultedActiveReplicas,
            writeQuorum_,
            avgReceiveAckDuration);

        if (toBeRestartedReplica == nullptr)
        {
            if (toBeRestartedReplica == nullptr)
            {
                // Quorum of replicas are probably slow or the idle is slow and it is not faulted due to some reason
                return;
            }
        }
            
        firstLsn = replicationQueueManager_.FirstLSNInReplicationQueue;
        lastLsn = replicationQueueManager_.GetLastSequenceNumber();

    } // release read lock

    if (toBeRestartedReplica)
    {
        // Ensure that queue is still at high usage before restarting to avoid races that could result in 2 secondary replicas to be restarted at the same time.
        // Restarting the slowest secondary could be sufficient for now and the restart of the next slowest secondary is strictly needed only after the queue fills up again
        {
            AcquireWriteLock grab(lock_);
 
            auto newReplicationQueueUsagePercent = (uint)perfCounters_->ReplicationQueueFullPercentage.Value;

            if (newReplicationQueueUsagePercent < config_->SlowActiveSecondaryRestartAtQueueUsagePercent)
            {
                // There was a racing thread that removed a secondary.
                return;
            }

            auto result = toBeRestartedReplica->TryFaultActiveReplicaDueToSlowProgress(
                avgApplyAckDuration,
                avgReceiveAckDuration,
                firstLsn,
                lastLsn);

            if (!result)
            {
                // Secondary Replica was closed or already restarted
                return;
            }

            UpdateProgressPrivateCallerHoldsLock(false);
            newReplicationQueueUsagePercent = (uint)perfCounters_->ReplicationQueueFullPercentage.Value;

            ReplicatorEventSource::Events->PrimaryCleanupQueueDueToSlowSecondary(
                partitionId_,
                endpointUniqueId_,
                config_->SlowActiveSecondaryRestartAtQueueUsagePercent,
                replicationQueueUsagePercent,
                newReplicationQueueUsagePercent,
                true);
        }
    }
}

void ReplicaManager::FaultSlowIdleReplicasIfPossible(uint replicationQueueUsagePercent)
{
    FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber;
    ReplicationSessionVector nonFaultedIdleReplicas;

    {
        AcquireReadLock grab(lock_);
        lastReplicationSequenceNumber = replicationQueueManager_.GetLastSequenceNumber();
        for (auto const & r : idleReplicas_)
        {
            if (!r->IsIdleFaultedDueToSlowProgress)
            {
                nonFaultedIdleReplicas.push_back(r);
            }
        }
    }

    if (nonFaultedIdleReplicas.empty())
    {
        return;
    }

    uint newReplicationQueueUsagePercent = replicationQueueUsagePercent;

    // First, check if extending the replication LSN of build completion helps in clearing out the queue
    if (Helpers::UpdateLastReplicationLSNsForIdleReplicas(nonFaultedIdleReplicas, lastReplicationSequenceNumber))
    {
        // There was atleast 1 idle, whose replicationLSN was increased. Lets try to updateprogress
        UpdateProgress();
        newReplicationQueueUsagePercent = (uint)perfCounters_->ReplicationQueueFullPercentage.Value;

        ReplicatorEventSource::Events->PrimaryCleanupQueueDueToSlowIdle(
            partitionId_,
            endpointUniqueId_,
            config_->SlowIdleRestartAtQueueUsagePercent,
            replicationQueueUsagePercent,
            newReplicationQueueUsagePercent,
            false);
    }

    bool faultedIdleReplicas = false;
    if (newReplicationQueueUsagePercent < static_cast<uint>(config_->SlowIdleRestartAtQueueUsagePercent))
    {
        // We have either cleared up some items or the user has disabled faulting idle replicas. So return back
        return;
    }

    // Acquiring lock and hence the indentation inside a scope
    {
        // Since we could not extend the replication LSN's anymore for the idles, we will try to fault the idle replicas if they are the 
        // cause for the queue being full
        // Fault the slowest idle replicas under the write lock as we could potentially be removing it

        AcquireReadLock grab(lock_);
        FABRIC_SEQUENCE_NUMBER firstReplicationSequenceNumber;

        lastReplicationSequenceNumber = replicationQueueManager_.GetLastSequenceNumber();
        firstReplicationSequenceNumber = replicationQueueManager_.FirstLSNInReplicationQueue;

        for (auto const & r : idleReplicas_)
        {
            // snap the lsn localy
            auto idleReplicaLsn = r->IdleReplicaProgress;

            // fault the replica if it is not already faulted
            if (idleReplicaLsn <= firstReplicationSequenceNumber &&
                !r->IsIdleFaultedDueToSlowProgress)
            {
                faultedIdleReplicas = faultedIdleReplicas |
                    r->TryFaultIdleReplicaDueToSlowProgress(
                        firstReplicationSequenceNumber,
                        lastReplicationSequenceNumber,
                        idleReplicaLsn);
            }
        }
    }

    if (faultedIdleReplicas)
    {
        // There was atleast 1 idle faulted. Update progress to check if it clears out the replication queue
        UpdateProgress();
        newReplicationQueueUsagePercent = (uint)perfCounters_->ReplicationQueueFullPercentage.Value;

        ReplicatorEventSource::Events->PrimaryCleanupQueueDueToSlowIdle(
            partitionId_,
            endpointUniqueId_,
            config_->SlowIdleRestartAtQueueUsagePercent,
            replicationQueueUsagePercent,
            newReplicationQueueUsagePercent,
            true);
    }
}

ErrorCode ReplicaManager::AddReplicateOperationPrivate(
    AsyncOperationSPtr const & thisSPtr,
    ComPointer<IFabricOperationData> && comOperationPointer,
    __out uint & replicationQueueUsagePercent,
    __out bool & operationCommitted,
    __out ComOperationCPtr & opComPtr)
{
    vector<AsyncOperationSPtr> operationsToComplete;
    ReplicationSessionVector sessions;
    operationCommitted = false;
    replicationQueueUsagePercent = 0;
    FABRIC_SEQUENCE_NUMBER currentCompletedLsn = Constants::InvalidLSN;

    {
        AcquireWriteLock lock(lock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->PrimaryReplMgrClosed(
                partitionId_,
                endpointUniqueId_,
                L"AddReplicateOperation");
            return ErrorCode(Common::ErrorCodeValue::ObjectClosed);
        }

        if (writeQuorum_ == 0)
        {
            return Common::ErrorCodeValue::ReconfigurationPending;
        }

        ErrorCode error = replicationQueueManager_.Enqueue(
            move(comOperationPointer),
            epoch_,
            opComPtr);
        
        replicationQueueUsagePercent = (uint)perfCounters_->ReplicationQueueFullPercentage.Value;

        if (replicationQueueUsagePercent >= config_->QueueHealthWarningAtUsagePercent)
        {
            wstring desc = Helpers::GetHealthReportWarningDescription(
                activeReplicas_,
                idleReplicas_,
                previousActiveReplicas_,
                replicationQueueManager_.FirstLSNInReplicationQueue,
                replicationQueueManager_.GetLastSequenceNumber(),
                replicationQueueUsagePercent,
                static_cast<uint>(config_->QueueHealthWarningAtUsagePercent));

            healthReporter_->ScheduleWarningReport(desc);
        }

        if (!error.IsSuccess())
        {
            if (error.IsError(Common::ErrorCodeValue::REQueueFull))
            {
                // Use the config always as it could have changed (dynamic config)
                if (lastQueueFullTraceWatch_.Elapsed > config_->QueueFullTraceInterval)
                {
                    lastQueueFullTraceWatch_.Restart();

                    // Trace below under the lock as the tracing method depends on it
                    ReplicatorEventSource::Events->PrimaryQueueFull(
                        partitionId_,
                        endpointUniqueId_,
                        *this,
                        replicationQueueManager_.GetLastSequenceNumber());
                }
            }

            return error;
        }

        // Keep track of the operation so it can be completed
        // when the associated queue entry is committed
        ASSERT_IF(
            replicateOperations_.find(opComPtr->SequenceNumber) != replicateOperations_.end(),
            "{0}: AddReplicateOperation: The operation {1} shouldn't exist",
            endpointUniqueId_,
            opComPtr->SequenceNumber);
        replicateOperations_.insert(
            std::pair<FABRIC_SEQUENCE_NUMBER, AsyncOperationSPtr>(opComPtr->SequenceNumber, thisSPtr));

        // Get the current list of replicas (Active and Idle)
        GetReplicasImpactingQueueUsageCallerHoldsLock(sessions);

        // If the primary is the only active replica in both CC and PC, 
        // the operation can be committed right away.
        // If the primary is the only replica,
        // the operation will also be completed.
        if (activeReplicas_.empty() && previousActiveReplicas_.empty())
        {
            operationCommitted = TryGetAndUpdateProgressCallerHoldsLock(operationsToComplete);
            if (operationCommitted)
            {
                CompleteReplicateOperationsCallerHoldsLock(operationsToComplete);
            }
        }

        currentCompletedLsn = replicationQueueManager_.FirstLSNInReplicationQueue - 1;
    }
    
    // Post the rest of the processing on a separate thread so that the BeginReplicate thread returns immediately
    // without issuing network send
    if (!sessions.empty())
    {
        ComOperationRawPtr opRawPtr = opComPtr.GetRawPointer();
        beginReplicateJobQueueUPtr_->Enqueue(ComReplicateJobPtr(move(opRawPtr), move(sessions), currentCompletedLsn));
    }
    
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ReplicationSessionSPtr ReplicaManager::GetReplica(
    std::wstring const & address,
    ReplicationEndpointId const & endpointUniqueId,
    __out FABRIC_REPLICA_ROLE & role)
{
    AcquireReadLock lock(lock_);
    if (!isActive_)
    {
        ReplicatorEventSource::Events->PrimaryReplMgrClosed(
            partitionId_,
            endpointUniqueId_,
            L"GetReplica");
        return nullptr;
    }
    
    ReplicationSessionSPtr session = Helpers::GetReplicaFromAddress(activeReplicas_, address, endpointUniqueId);
    if (session)
    {
        role = FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY;
        return session;
    }

    // Look in the previous configuration if it's still active
    if (usePreviousActiveReplicas_)
    {
        session = Helpers::GetReplicaFromAddress(previousActiveReplicas_, address, endpointUniqueId);
        if (session)
        {
            role = FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY;
            return session;
        }
    }

    role = FABRIC_REPLICA_ROLE_IDLE_SECONDARY;
    return Helpers::GetReplicaFromAddress(idleReplicas_, address, endpointUniqueId);
}

// *****************************
// Methods that need both 
// replication queue and sessions
// *****************************
bool ReplicaManager::UpdateProgress()
{
    AcquireWriteLock lock(lock_);
    return UpdateProgressPrivateCallerHoldsLock(false);
}

bool ReplicaManager::UpdateProgressPrivateCallerHoldsLock(bool forceUpdate)
{
    vector<Common::AsyncOperationSPtr> operationsToComplete;

    if (!isActive_ && 
        forceUpdate == false)
    {
        ReplicatorEventSource::Events->PrimaryReplMgrClosed(
            partitionId_,
            endpointUniqueId_,
            L"UpdateProgress");
        return false;
    }

    bool progressMade = TryGetAndUpdateProgressCallerHoldsLock(operationsToComplete);

    if (forceUpdate)
    {
        ASSERT_IF(
            operationsToComplete.size() != 0,
            "{0}- Force Update progress during close is not expected to have any replication operations to complete",
            endpointUniqueId_);
    }

    if (progressMade)
    {
        CompleteReplicateOperationsCallerHoldsLock(operationsToComplete);
    }

    return true;
}

void ReplicaManager::CompleteReplicateOperationsCallerHoldsLock(
    vector<Common::AsyncOperationSPtr> & operationsToComplete)
{
    for (auto iterator = operationsToComplete.begin();
        iterator != operationsToComplete.end();
        iterator++)
    { 
        completeReplicateJobQueueUPtr_->Enqueue(AsyncOperationSPtrJob(std::move(*iterator))); 
    }
}

// Used by ReplicaManager CIT
bool ReplicaManager::Test_TryGetProgress(
    __out wstring & currentActiveReplicas,
    __out wstring & previousActiveReplicas,
    __out wstring & idleReplicas,
    __out FABRIC_SEQUENCE_NUMBER & committedLSN,
    __out FABRIC_SEQUENCE_NUMBER & completedLSN,
    __out bool & replicationAckProcessingInProgress)
{
    AcquireWriteLock lock(lock_); // The GetProgressCallerHoldsLock updates some state
    replicationAckProcessingInProgress = false;
    ASSERT_IFNOT(
        isActive_,
        "{0}: Test_TryGetProgress: replica manager is not Opened",
        endpointUniqueId_);

    currentActiveReplicas = move(Helpers::GetReplicaIdsString(activeReplicas_));
    previousActiveReplicas = move(Helpers::GetReplicaIdsString(previousActiveReplicas_));
    idleReplicas = move(Helpers::GetReplicaIdsString(idleReplicas_));
    
    if (!hasEnoughReplicasForQuorum_ || 
        (usePreviousActiveReplicas_ && !hasEnoughReplicasForQuorumInPreviousConfig_))
    {
        // There are not enough replicas in current configuration
        // Or a reconfiguration is in progress and the previous config doesn't have quorum;
        // No progress can be made.
        return false;
    }
    else
    {
        for (auto s : activeReplicas_)
        {
            replicationAckProcessingInProgress |= s->Test_IsReplicationAckProcessingInProgress();
        }
        for (auto s : previousActiveReplicas_)
        {
            replicationAckProcessingInProgress |= s->Test_IsReplicationAckProcessingInProgress();
        }
        for (auto s : idleReplicas_)
        {
            if (!s->IsIdleFaultedDueToSlowProgress)
            {
                replicationAckProcessingInProgress |= s->Test_IsReplicationAckProcessingInProgress();
            }
        }

        GetProgressCallerHoldsLock(committedLSN, completedLSN);
        return true;
    }
}

bool ReplicaManager::TryGetAndUpdateProgressCallerHoldsLock(
    __out vector<Common::AsyncOperationSPtr> & operationsToComplete)
{
    if (!hasEnoughReplicasForQuorum_)
    {
        // There are not enough replicas in current configuration
        // No progress can be made.
        return false;
    }

    // Update progress for the replication queue
    FABRIC_SEQUENCE_NUMBER committedLSN;
    FABRIC_SEQUENCE_NUMBER completedLSN;
    GetProgressCallerHoldsLock(committedLSN, completedLSN);
        
    if (usePreviousActiveReplicas_ && !hasEnoughReplicasForQuorumInPreviousConfig_)
    {
        // A reconfiguration is in progress and the previous config doesn't have quorum;
        // In this case, the current committed and completed numbers must have been set,
        // but there no updates need to happen
        return false;
    }

    if (!config_->AllowMultipleQuorumSet && updatingCatchUpReplicaSetConfiguration_)
    {
        // In case multiple quorum set is disabled, and deactivate phase is not completed,
        // we only update operation queue pointers up to committed LSN in PC.
        replicationQueueManager_.CheckWithCatchupLsn(committedLSN);
        replicationQueueManager_.CheckWithCatchupLsn(completedLSN);
    }

    FABRIC_SEQUENCE_NUMBER oldCommittedLSN;
    FABRIC_SEQUENCE_NUMBER newCommittedLSN;
    bool removedItemsFromQueue = false;
    UpdateProgressInQueueCallerHoldsLock(committedLSN, completedLSN, oldCommittedLSN, newCommittedLSN, removedItemsFromQueue);

    if (removedItemsFromQueue)
    {
        // In most cases, removing items from the queue implies the queue full condition is relieved
        // It may not be true in cases where queue memory limit is hit and not operation count. But, we have no way to predict the
        // memory size of the incoming operations.
        if (perfCounters_->ReplicationQueueFullPercentage.Value < config_->QueueHealthWarningAtUsagePercent)
        {
            healthReporter_->ScheduleOKReport();
        }
    }

    if (oldCommittedLSN < newCommittedLSN)
    {
        operationsToComplete.reserve(newCommittedLSN - oldCommittedLSN);

        ReplicatorEventSource::Events->PrimaryReplicateDone(
            partitionId_,
            endpointUniqueId_, 
            oldCommittedLSN + 1, 
            newCommittedLSN - oldCommittedLSN);
    
        // Remember all async replicate operations up to the latest committed sequence number
        // to complete them 
        for (FABRIC_SEQUENCE_NUMBER i = oldCommittedLSN + 1; i <= newCommittedLSN; ++i)
        {
            auto it = replicateOperations_.find(i);
            // If not found, the operation has already been committed
            // if the commit head was moved back after adding another secondary
            if (it != replicateOperations_.end())
            {
                operationsToComplete.push_back(move(it->second));
                replicateOperations_.erase(it);
            }
        }
    }

    if (waitForPendingOperationsToCompleteEventSPtr_ &&
        replicateOperations_.size() == 0)
    {
        ReplicatorEventSource::Events->PrimaryPendingReplicateOperationsCompleted(
            partitionId_,
            endpointUniqueId_);

        waitForPendingOperationsToCompleteEventSPtr_->Set();
    }

    return true;
}

void ReplicaManager::GetProgressCallerHoldsLock(
    __out FABRIC_SEQUENCE_NUMBER & committedLSN,
    __out FABRIC_SEQUENCE_NUMBER & completedLSN)
{
    if (!hasEnoughReplicasForQuorum_)
    {
        Assert::CodingError(
            "{0}: GetProgressCallerHoldsLock shouldn't be called if there are not enough replicas - current {1}, reconfig in progress {2}, previous {3}",
            endpointUniqueId_,
            hasEnoughReplicasForQuorum_,
            usePreviousActiveReplicas_,
            hasEnoughReplicasForQuorumInPreviousConfig_);
    }
    
    FABRIC_SEQUENCE_NUMBER allAckedLSN;
    auto currentAllAckLSN = replicationQueueManager_.FirstLSNInReplicationQueue - 1;

    Helpers::GetReplicasProgress(
        currentAllAckLSN,
        activeReplicas_, 
        writeQuorum_, 
        endpointUniqueId_, 
        committedLSN, 
        allAckedLSN, 
        completedLSN);

    // Check the idle replicas 
    if (!idleReplicas_.empty())
    {
        vector<FABRIC_SEQUENCE_NUMBER> idleSessionsProgressLSNs;
        for (auto const & r : idleReplicas_)
        {
            if (!r->IsIdleFaultedDueToSlowProgress)
            {
                idleSessionsProgressLSNs.push_back(r->IdleReplicaProgress);
            }
        }
        
        if (!idleSessionsProgressLSNs.empty())
        {
            auto itMin = std::min_element(idleSessionsProgressLSNs.begin(), idleSessionsProgressLSNs.end());
            if (completedLSN == Constants::NonInitializedLSN || *itMin < completedLSN)
            {
                completedLSN = *itMin;
            }
        }
    }

    // Save the numbers in the current replicas numbers;
    // they will be used to determine if catch up operations should complete
    // when reconfiguration is in progress.
    majorityReplicasApplyAckedLSNinCC_ = committedLSN;
    allReplicasApplyAckedLSNinCC_ = allAckedLSN;

    // If a reconfiguration is in progress, compute the committed number
    // for the old configuration as well,
    // and update the returned numbers.
    // This can only be done if we have enough quorum
    if (usePreviousActiveReplicas_ && hasEnoughReplicasForQuorumInPreviousConfig_)
    {
        FABRIC_SEQUENCE_NUMBER previousCommittedLSN;
        FABRIC_SEQUENCE_NUMBER previousCompletedLSN;

        if (previousWriteQuorum_ == 1 &&
            !previousActiveReplicas_.empty())
        {
            //***********************************************************************************************
            // NOTE - In the rest of the code, the replicator assumes that the local primary is always part of the quorum in PC and CC 
            //***********************************************************************************************
            //
            // Here, there is a single non primary replica in PC when write quorum is 1 (I/P,S/N transition)
            // The committed and completed LSN should be the last ACK'd LSN of the lone replica in PC

            // While this situation (Primary not part of PC) can happen any time, we special case our progress calculation ONLY when previousWriteQuorum_ = 1
            // because it is only here that we discard all operations( since committed and completed LSN is -1 for writeQuorum = 1)
            // In other cases, we still keep the operations in the primary queue as they are not yet ACK'd by all replicas
            
            // Ascending sort the PC list by the LSN, then use the index 0 as the progress
            vector<FABRIC_SEQUENCE_NUMBER> lastAckedLSNs = Helpers::GetSortedLSNList(
                currentAllAckLSN,
                previousActiveReplicas_,
                false /*useReceiveAck*/,
                [](FABRIC_SEQUENCE_NUMBER i, FABRIC_SEQUENCE_NUMBER j) 
                    { return i < j; });

            previousCommittedLSN = lastAckedLSNs[0];
            previousCompletedLSN = lastAckedLSNs[0];
        }
        else
        {
            Helpers::GetReplicasProgress(
                currentAllAckLSN,
                previousActiveReplicas_, 
                previousWriteQuorum_, 
                endpointUniqueId_, 
                previousCommittedLSN, 
                allAckedLSN, 
                previousCompletedLSN);
        }

        if (previousCommittedLSN != Constants::NonInitializedLSN && 
            (committedLSN == Constants::NonInitializedLSN || previousCommittedLSN < committedLSN))
        {
            committedLSN = previousCommittedLSN;
        }

        if (previousCompletedLSN != Constants::NonInitializedLSN && 
            (completedLSN == Constants::NonInitializedLSN || previousCompletedLSN < completedLSN))
        {
            completedLSN = previousCompletedLSN;
        }
    }
}

void ReplicaManager::UpdateProgressInQueueCallerHoldsLock(
    FABRIC_SEQUENCE_NUMBER committedLSN,
    FABRIC_SEQUENCE_NUMBER completedLSN,
    __out FABRIC_SEQUENCE_NUMBER & oldCommittedLSN,
    __out FABRIC_SEQUENCE_NUMBER & newCommittedLSN,
    __out bool & removedItemsFromQueue)
{
    ReplicatorEventSource::Events->PrimaryUpdateReplQueue(
        partitionId_,
        endpointUniqueId_, 
        committedLSN, 
        completedLSN);

    replicationQueueManager_.UpdateQueue(
        committedLSN, 
        completedLSN, 
        oldCommittedLSN,
        newCommittedLSN,
        removedItemsFromQueue);
}

// *****************************
// Helper methods
// *****************************
void ReplicaManager::GetReplicasImpactingQueueUsageCallerHoldsLock(__out ReplicationSessionVector & sessions) const
{
    for (auto const & r : activeReplicas_)
    {
        sessions.push_back(r);
    }

    for (auto const & r : idleReplicas_)
    {
        if (!r->IsIdleFaultedDueToSlowProgress)
        {
            sessions.push_back(r);
        }
    }

    if (usePreviousActiveReplicas_)
    {
        // Some of the replicas may already be in the active replicas
        // so only add the diff
        for (auto itPrev = previousActiveReplicas_.begin(); itPrev != previousActiveReplicas_.end(); ++itPrev)
        {
            FABRIC_REPLICA_ID replicaId = (*itPrev)->ReplicaId;
            auto it = std::find_if(activeReplicas_.begin(), activeReplicas_.end(),
                [replicaId](ReplicationSessionSPtr const & r)->bool
                {
                    return r->ReplicaId == replicaId;
                });
            if (it == activeReplicas_.end())
            {
                sessions.push_back(*itPrev);
            }
        }
    }
}

bool ReplicaManager::FindReplicaCallerHoldsLock(ReplicaInformation const & replica, ReplicationSessionSPtr & session)
{
    // Look for the replica with specified endpoint
    // in the list of active replicas (in previous or current configuration). 
    // If not found, look at the idle replicas.
    session = Helpers::GetReplica(activeReplicas_, replica);
    if (session)
    {
        return true;
    }
    
    session = Helpers::GetReplica(idleReplicas_, replica);
    if (session)
    {
        return true;
    }

    if (usePreviousActiveReplicas_)
    {
        session = Helpers::GetReplica(previousActiveReplicas_, replica);
    }

    return session != nullptr;
}
 
wstring ReplicaManager::ToString() const
{
    std::wstring content;
    Common::StringWriter writer(content);
    WriteTo(writer, Common::FormatOptions(0, false, ""));        
    return content;
}

void ReplicaManager::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    // Don't acquire any locks as the trace call is under the lock
    ReplicationSessionVector nonFaultedReplicas;
    StandardDeviation receiveAck, applyAck;
 
    GetReplicasImpactingQueueUsageCallerHoldsLock(nonFaultedReplicas);

    if (nonFaultedReplicas.size() > 0)
    {
        applyAck = Helpers::ComputeAverageAckDuration(
            nonFaultedReplicas,
            nonFaultedReplicas.size() - 1,
            false);

        receiveAck = Helpers::ComputeAverageAckDuration(
            nonFaultedReplicas,
            nonFaultedReplicas.size() - 1,
            true);
    }

    w.Write(
        "\r\nAvgReceiveDur={0}ms (SD={1}ms), AvgApplyDur={2}ms (SD={3}ms)", 
        receiveAck.Average.TotalMilliseconds(), 
        receiveAck.StdDev.TotalMilliseconds(), 
        applyAck.Average.TotalMilliseconds(), 
        applyAck.StdDev.TotalMilliseconds());

    w.Write("\r\nactive:\r\n");
    w.Write(activeReplicas_);
    w.Write("\r\nidle:\r\n");
    w.Write(idleReplicas_);
    if (usePreviousActiveReplicas_)
    {
        w.Write("\r\nprevious active:\r\n");
        w.Write(previousActiveReplicas_);
    }
}

std::string ReplicaManager::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    std::string format = "\r\nAvgReceiveDur={0}ms (SD={1}ms), AvgApplyDur={2}ms (SD={3}ms) \r\nactive:\r\n{4}\r\nidle:\r\n{5}\r\nprevious:\r\n{6}\r\n";
    size_t index = 0;

    traceEvent.AddEventField<int64>(format, name + ".avgReceiveAckDur", index);
    traceEvent.AddEventField<int64>(format, name + ".receiveAckSD", index);
    traceEvent.AddEventField<int64>(format, name + ".avgApplyAckDur", index);
    traceEvent.AddEventField<int64>(format, name + ".applyAckSD", index);
    traceEvent.AddEventField<ReplicationSessionVector>(format, name + ".active", index);
    traceEvent.AddEventField<ReplicationSessionVector>(format, name + ".idle", index);
    traceEvent.AddEventField<ReplicationSessionVector>(format, name + ".previous", index);

    return format;
}

void ReplicaManager::FillEventData(Common::TraceEventContext & context) const
{
    // Don't acquire any locks as the trace call is under the lock
    ReplicationSessionVector nonFaultedReplicas;
    StandardDeviation receiveAck, applyAck;
 
    GetReplicasImpactingQueueUsageCallerHoldsLock(nonFaultedReplicas);

    if (nonFaultedReplicas.size() > 0)
    {
        applyAck = Helpers::ComputeAverageAckDuration(
            nonFaultedReplicas,
            nonFaultedReplicas.size() - 1,
            false);

        receiveAck = Helpers::ComputeAverageAckDuration(
            nonFaultedReplicas,
            nonFaultedReplicas.size() - 1,
            true);
    }

    context.WriteCopy<int64>(receiveAck.Average.TotalMilliseconds());
    context.WriteCopy<int64>(receiveAck.StdDev.TotalMilliseconds());
    context.WriteCopy<int64>(applyAck.Average.TotalMilliseconds());
    context.WriteCopy<int64>(applyAck.StdDev.TotalMilliseconds());
    context.Write<ReplicationSessionVector>(activeReplicas_);
    context.Write<ReplicationSessionVector>(idleReplicas_);
    context.Write<ReplicationSessionVector>(previousActiveReplicas_);
}

} // end namespace ReplicationComponent
} // end namespace Reliability
