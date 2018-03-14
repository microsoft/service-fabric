// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;
using namespace std;

namespace Store
{
    StringLiteral const TraceComponent("EseReplicatedStoreRepairPolicy");

    // 
    // OnOpen policy
    //

    class EseReplicatedStoreRepairPolicy::GetRepairActionOnOpenAsyncOperation : public AsyncOperation
    {
    public:

        GetRepairActionOnOpenAsyncOperation(
            __in EseReplicatedStoreRepairPolicy & owner,
            RepairDescription const & description,
            TimeSpan const & timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr  const & parent)
            : AsyncOperation(callback, parent)
            , owner_(owner)
            , description_(description)
            , timeoutHelper_(timeout)
            , targetReplicaSetSize_(0)
            , repairAction_(RepairAction::None)
        {
        }

        __declspec(property(get=get_TraceId)) wstring const & TraceId;
        wstring const & get_TraceId() const { return dynamic_cast<PartitionedReplicaTraceComponent&>(owner_).TraceId; }

        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            this->Evaluate(thisSPtr);
        }

        static ErrorCode End(AsyncOperationSPtr const & operation, __out RepairAction::Enum & result)
        {
            auto casted = AsyncOperation::End<GetRepairActionOnOpenAsyncOperation>(operation);

            if (casted->Error.IsSuccess())
            {
                result = casted->repairAction_;
            }

            return casted->Error;;
        }

    private:

        void Evaluate(AsyncOperationSPtr const & thisSPtr)
        {
            if (this->IsValidRepairCondition())
            {
                this->CheckPartition(thisSPtr);
            }
            else
            {
                owner_.WriteInfo(
                    TraceComponent,
                    "{0} cannot repair: condition={1} role={2}",
                    this->TraceId,
                    description_.RepairCondition,
                    description_.ReplicaRole);

                this->Complete(thisSPtr, RepairAction::None);
            }
        }

        bool IsValidRepairCondition()
        {
            switch (description_.RepairCondition)
            {
            case RepairCondition::CorruptDatabaseLogs:
            case RepairCondition::DatabaseFilesAccessDenied:
                //
                // The only automatic repair we currently support is dropping 
                // corrupt database logs and handling database file access issues
                // encountered during open
                //
                return (description_.ReplicaRole == ::FABRIC_REPLICA_ROLE_UNKNOWN);

            default:
                return false;
            }
        }

        void CheckPartition(AsyncOperationSPtr const & thisSPtr)
        {
            auto operation = owner_.queryClientPtr_->BeginGetPartitionList(
                owner_.ServiceName,
                owner_.PartitionId,
                L"", // continuationToken
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
                { 
                    this->OnGetPartitionListComplete(operation, false); 
                },
                thisSPtr);
            this->OnGetPartitionListComplete(operation, true);
        }

        void OnGetPartitionListComplete(
            AsyncOperationSPtr const & operation, 
            bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            auto const & thisSPtr = operation->Parent;

            vector<ServicePartitionQueryResult> partitionList;
            PagingStatusUPtr pagingStatus;
            auto error = owner_.queryClientPtr_->EndGetPartitionList(operation, partitionList, pagingStatus);

            if (!error.IsSuccess())
            {
                owner_.WriteInfo(
                    TraceComponent,
                    "{0} partition query failed: error={1}",
                    this->TraceId,
                    error);

                this->TryComplete(thisSPtr, error);

                return;
            }

            auto findIt = find_if(partitionList.begin(), partitionList.end(),
                [&](ServicePartitionQueryResult const & item)
                {
                    return (item.PartitionId == owner_.PartitionId);
                });

            if (findIt == partitionList.end())
            {
                owner_.WriteInfo(
                    TraceComponent,
                    "{0} partition not found in query results: partition={1}",
                    this->TraceId,
                    owner_.PartitionId);

                this->Complete(thisSPtr, RepairAction::None);

                return;
            }

            ServicePartitionQueryResult const & partitionResult = *findIt;

            // No repair needed for healthy partitions.
            //
            // Be conservative and do not perform automatic repairs if the partition is in error
            // or QL.
            //
            if (partitionResult.HealthState != ::FABRIC_HEALTH_STATE_WARNING || partitionResult.InQuorumLoss)
            {
                owner_.WriteInfo(
                    TraceComponent,
                    "{0} cannot repair: health={1} QL={2}",
                    this->TraceId,
                    partitionResult.HealthState,
                    partitionResult.InQuorumLoss);

                this->Complete(thisSPtr, RepairAction::None);

                return;
            }

            // Do not automatically repair partitions with fewer than 3 replicas since
            // we could mess up the only replica (or the only backup)
            //
            targetReplicaSetSize_ = 3;

            if (partitionResult.TargetReplicaSetSize > targetReplicaSetSize_)
            {
                targetReplicaSetSize_ = partitionResult.TargetReplicaSetSize;
            }

            this->CheckReplicas(thisSPtr);
        }

        void CheckReplicas(AsyncOperationSPtr const & thisSPtr)
        {
            auto operation = owner_.queryClientPtr_->BeginGetReplicaList(
                owner_.PartitionId,
                0, // replica ID
                ::FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_READY,
                L"", // continuationToken
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
                { 
                    this->OnGetReplicaListComplete(operation, false); 
                },
                thisSPtr);
            this->OnGetReplicaListComplete(operation, true);
        }

        void OnGetReplicaListComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            auto const & thisSPtr = operation->Parent;

            vector<ServiceReplicaQueryResult> replicaList;
            PagingStatusUPtr pagingStatus;
            auto error = owner_.queryClientPtr_->EndGetReplicaList(operation, replicaList, pagingStatus);

            if (!error.IsSuccess())
            {
                owner_.WriteInfo(
                    TraceComponent,
                    "{0} replica query failed: error={1}",
                    this->TraceId,
                    error);

                this->TryComplete(thisSPtr, error);

                return;
            }

            // Be conservative and only repair this replica if no other replicas
            // in the set are down or unhealthy. The minimum target we accept is 3,
            // so a minimum of 2 replicas must be healthy.
            //
            uint healthyCount = 0;
            for (auto const & replica : replicaList)
            {
                if (replica.HealthState == ::FABRIC_HEALTH_STATE_OK)
                {
                    ++healthyCount;
                }
            }

            uint repairLimit = owner_.allowRepairUpToQuorum_ ? (targetReplicaSetSize_ / 2 + 1) : (targetReplicaSetSize_ - 1);

            if (healthyCount < repairLimit)
            {
                owner_.WriteInfo(
                    TraceComponent,
                    "{0} cannot repair: replica count(healthy={1} target={2} limit={3})",
                    this->TraceId,
                    healthyCount,
                    targetReplicaSetSize_,
                    repairLimit);

                this->Complete(thisSPtr, RepairAction::None);

                return;
            }

            auto findIt = find_if(replicaList.begin(), replicaList.end(),
                [&](ServiceReplicaQueryResult const & item)
                {
                    return (item.ReplicaRole == ::FABRIC_REPLICA_ROLE_PRIMARY);
                });

            // Primary must be up to perform automatic repairs
            //
            if (findIt == replicaList.end())
            {
                owner_.WriteInfo(
                    TraceComponent,
                    "{0} cannot repair: primary not found in replica query",
                    this->TraceId);

                this->Complete(thisSPtr, RepairAction::None);

                return;
            }
            
            // Passed all checks - recommend automatic repair attempt
            //
            auto repairAction = GetRepairAction();

            owner_.WriteInfo(
                TraceComponent,
                "{0} repair policy: action={1}",
                this->TraceId,
                repairAction);

            this->Complete(thisSPtr, repairAction);
        }

        RepairAction::Enum GetRepairAction()
        {
            switch (description_.RepairCondition)
            {
            case RepairCondition::CorruptDatabaseLogs:
                return RepairAction::DropDatabase;

            case RepairCondition::DatabaseFilesAccessDenied:
                return RepairAction::TerminateProcess;

            default:
                return RepairAction::None;
            }
        }

        void Complete(
            AsyncOperationSPtr const & thisSPtr,
            RepairAction::Enum action)
        {
            repairAction_ = action;

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }

        EseReplicatedStoreRepairPolicy & owner_;
        RepairDescription description_;
        TimeoutHelper timeoutHelper_;
        uint targetReplicaSetSize_;
        RepairAction::Enum repairAction_;
    };
    
    //
    // *** EseReplicatedStoreRepairPolicy
    //

    EseReplicatedStoreRepairPolicy::EseReplicatedStoreRepairPolicy(
        __in IQueryClientPtr & queryClientPtr,
        NamingUri const & serviceName,
        bool allowRepairUpToQuorum,
        PartitionedReplicaTraceComponent const & traceComponent)
        : PartitionedReplicaTraceComponent(traceComponent)
        , queryClientPtr_(queryClientPtr)
        , serviceName_(serviceName)
        , allowRepairUpToQuorum_(allowRepairUpToQuorum)
    {
    }

    ErrorCode EseReplicatedStoreRepairPolicy::Create(
        Api::IClientFactoryPtr const & clientFactoryPtr,
        NamingUri const & serviceName,
        bool allowRepairUpToQuorum,
        PartitionedReplicaTraceComponent const & traceComponent,
        __out shared_ptr<IRepairPolicy> & result)
    {
        IQueryClientPtr queryClientPtr;
        auto error = clientFactoryPtr->CreateQueryClient(queryClientPtr);

        if (error.IsSuccess())
        {
            result = shared_ptr<EseReplicatedStoreRepairPolicy>(new EseReplicatedStoreRepairPolicy(
                queryClientPtr,
                serviceName,
                allowRepairUpToQuorum,
                traceComponent));
        }

        return error;
    }

    AsyncOperationSPtr EseReplicatedStoreRepairPolicy::BeginGetRepairActionOnOpen(
        RepairDescription const & description,
        TimeSpan const & timeout,
        __in AsyncCallback const & callback,
        __in AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<GetRepairActionOnOpenAsyncOperation>(
            *this,
            description,
            timeout,
            callback,
            parent);
    }

    ErrorCode EseReplicatedStoreRepairPolicy::EndGetRepairActionOnOpen(
        __in AsyncOperationSPtr const & operation,
        __out RepairAction::Enum & result)
    {
        return GetRepairActionOnOpenAsyncOperation::End(operation, result);
    }
}
