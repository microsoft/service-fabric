// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DeployedServiceReplicaDetailQueryResult::DeployedServiceReplicaDetailQueryResult()
    : kind_(FABRIC_SERVICE_KIND_INVALID)
    , serviceName_()
    , partitionId_(Guid::Empty())
    , currentServiceOperation_(FABRIC_QUERY_SERVICE_OPERATION_NAME_INVALID)
    , currentServiceOperationStartTime_(DateTime::Zero)
    , reportedLoad_()
    , instanceId_(-1)
    , replicaId_(-1)
    , currentReplicatorOperation_(FABRIC_QUERY_REPLICATOR_OPERATION_NAME_INVALID)
    , readStatus_(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID)
    , writeStatus_(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID)
    , replicatorStatus_()
    , replicaStatus_()
    , deployedServiceReplicaQueryResult_()
{
}

// Stateless
DeployedServiceReplicaDetailQueryResult::DeployedServiceReplicaDetailQueryResult(
    std::wstring const & serviceName,
    Common::Guid const & partitionId,
    int64 instanceId,
    FABRIC_QUERY_SERVICE_OPERATION_NAME currentServiceOperation,
    Common::DateTime currentServiceOperationStartTime,
    std::vector<ServiceModel::LoadMetricReport> && reportedLoad)
    : kind_(FABRIC_SERVICE_KIND_STATELESS)
    , serviceName_(serviceName)
    , partitionId_(partitionId)
    , currentServiceOperation_(currentServiceOperation)
    , currentServiceOperationStartTime_(currentServiceOperationStartTime)
    , reportedLoad_(move(reportedLoad))
    , instanceId_(instanceId)
    , replicaId_(-1)
    , currentReplicatorOperation_(FABRIC_QUERY_REPLICATOR_OPERATION_NAME_INVALID)
    , readStatus_(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID)
    , writeStatus_(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID)
    , replicatorStatus_()
    , replicaStatus_()
    , deployedServiceReplicaQueryResult_()
{
}

// Stateful
DeployedServiceReplicaDetailQueryResult::DeployedServiceReplicaDetailQueryResult(
    std::wstring const & serviceName,
    Common::Guid const & partitionId,
    int64 replicaId,
    FABRIC_QUERY_SERVICE_OPERATION_NAME currentServiceOperation,
    Common::DateTime currentServiceOperationStartTime,
    FABRIC_QUERY_REPLICATOR_OPERATION_NAME currentReplicatorOperation,
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus,
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus,
    std::vector<ServiceModel::LoadMetricReport> && reportedLoad)
    : kind_(FABRIC_SERVICE_KIND_STATEFUL)
    , serviceName_(serviceName)
    , partitionId_(partitionId)
    , currentServiceOperation_(currentServiceOperation)
    , currentServiceOperationStartTime_(currentServiceOperationStartTime)
    , reportedLoad_(move(reportedLoad))
    , instanceId_(-1)
    , replicaId_(replicaId)
    , currentReplicatorOperation_(currentReplicatorOperation)
    , readStatus_(readStatus)
    , writeStatus_(writeStatus)
    , deployedServiceReplicaQueryResult_()
{
}

void DeployedServiceReplicaDetailQueryResult::SetReplicatorStatusQueryResult(ReplicatorStatusQueryResultSPtr && result)
{
    ASSERT_IF(result == nullptr, "Can't set null result - if the replicator failed to provide a result then it should have returned an error");
    ASSERT_IF(replicatorStatus_ != nullptr, "Can't set replicator result multiple times");
    replicatorStatus_ = move(result);
}

void DeployedServiceReplicaDetailQueryResult::SetReplicaStatusQueryResult(ReplicaStatusQueryResultSPtr && result)
{
    ASSERT_IF(result == nullptr, "Can't set null result - if the replica failed to provide a result then it should have returned an error");
    ASSERT_IF(replicaStatus_ != nullptr, "Can't set replica result multiple times");
    replicaStatus_ = move(result);
}

void DeployedServiceReplicaDetailQueryResult::SetDeployedServiceReplicaQueryResult(ServiceModel::DeployedServiceReplicaQueryResultSPtr && result)
{
    deployedServiceReplicaQueryResult_ = result;
}

ErrorCode DeployedServiceReplicaDetailQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM & publicResult)
{
    publicResult.Kind = kind_;
    publicResult.Value = nullptr;

    if (kind_ == FABRIC_SERVICE_KIND_STATEFUL)
    {
        auto result = heap.AddItem<FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM>();
        result->ServiceName = heap.AddString(serviceName_);
        result->PartitionId = partitionId_.AsGUID();
        result->ReplicaId= replicaId_;
        result->CurrentServiceOperation = currentServiceOperation_;
        result->CurrentServiceOperationStartTimeUtc = currentServiceOperationStartTime_.AsFileTime;
        
        auto list = heap.AddItem<FABRIC_LOAD_METRIC_REPORT_LIST>();
        PublicApiHelper::ToPublicApiList<LoadMetricReport, FABRIC_LOAD_METRIC_REPORT, FABRIC_LOAD_METRIC_REPORT_LIST>(heap, reportedLoad_, *list);
        result->ReportedLoad = list.GetRawPointer();

        result->CurrentReplicatorOperation = currentReplicatorOperation_;
        result->ReadStatus = readStatus_;
        result->WriteStatus = writeStatus_;

        if (replicatorStatus_ != nullptr)
        {
            auto replicatorStatus = heap.AddItem<FABRIC_REPLICATOR_STATUS_QUERY_RESULT>();
            replicatorStatus_->ToPublicApi(heap, *replicatorStatus);
            result->ReplicatorStatus = replicatorStatus.GetRawPointer();
        }

        auto ex1 = heap.AddItem<FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM_EX1>();

        if (replicaStatus_ != nullptr)
        {
            auto replicaStatus = heap.AddItem<FABRIC_REPLICA_STATUS_QUERY_RESULT>();
            replicaStatus_->ToPublicApi(heap, *replicaStatus);

            ex1->ReplicaStatus = replicaStatus.GetRawPointer();
        }

        if(deployedServiceReplicaQueryResult_ != nullptr)
        {
            auto deployedStatefulServiceReplica = heap.AddItem<FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM>();
            deployedServiceReplicaQueryResult_->ToPublicApi(heap, *deployedStatefulServiceReplica);

            auto extended2 = heap.AddItem<FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM_EX2>();
            extended2->DeployedServiceReplica = deployedStatefulServiceReplica.GetRawPointer();

            ex1->Reserved = extended2.GetRawPointer();
        }

        result->Reserved = ex1.GetRawPointer();
        publicResult.Value = result.GetRawPointer();
    }
    else if (kind_ == FABRIC_SERVICE_KIND_STATELESS)
    {
        auto result = heap.AddItem<FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_DETAIL_QUERY_RESULT_ITEM>();
        result->ServiceName = heap.AddString(serviceName_);
        result->PartitionId = partitionId_.AsGUID();
        result->InstanceId = instanceId_;
        result->CurrentServiceOperation = currentServiceOperation_;
        result->CurrentServiceOperationStartTimeUtc = currentServiceOperationStartTime_.AsFileTime;
        
        auto list = heap.AddItem<FABRIC_LOAD_METRIC_REPORT_LIST>();
        PublicApiHelper::ToPublicApiList<LoadMetricReport, FABRIC_LOAD_METRIC_REPORT, FABRIC_LOAD_METRIC_REPORT_LIST>(heap, reportedLoad_, *list);
        result->ReportedLoad = list.GetRawPointer();

        if(deployedServiceReplicaQueryResult_ != nullptr)
        {
            auto extended1 = heap.AddItem<FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_DETAIL_QUERY_RESULT_ITEM_EX1>();

            auto deployedServiceStatelessReplica = heap.AddItem<FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM>();
            deployedServiceReplicaQueryResult_->ToPublicApi(heap, *deployedServiceStatelessReplica);
            extended1->DeployedServiceReplica = deployedServiceStatelessReplica.GetRawPointer();

            result->Reserved = extended1.GetRawPointer();
        }

        publicResult.Value = result.GetRawPointer();
    }
    else
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCode::Success();
}

void DeployedServiceReplicaDetailQueryResult::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << ToString();
}

std::wstring DeployedServiceReplicaDetailQueryResult::ToString() const
{
    auto part1 = wformatString("Kind = {0}, Name = {1}, PartitionId = {2}, InstanceId = {3}, ReplicaId = {4}, CurrentServiceOp = {5}, CurrentServiceOpStartTime = {6}, LoadCount = {7}",
        kind_,
        serviceName_,
        partitionId_,
        instanceId_,
        replicaId_,
        currentServiceOperation_,
        currentServiceOperationStartTime_,
        reportedLoad_.size());

    auto part2 = wformatString("ReplicatorOperationName = {0}, ReadStatus = {1}, WriteStatus = {2}, ReplicatorStatus = {3}",
        currentReplicatorOperation_,
        readStatus_,
        writeStatus_,
        replicatorStatus_ == nullptr ? L"null" : replicatorStatus_->ToString());

    auto part3 = wformatString("ReplicaStatus = {0}", replicaStatus_ ? replicaStatus_->ToString() : L"null");

    return wformatString("{0}, {1}, {2}", part1, part2, part3);
}
