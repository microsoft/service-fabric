// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;
using namespace ServiceModel;
using namespace Query;
using namespace Naming;
using namespace TestCommon;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace Api;

const StringLiteral TraceSource("FabricTest.QueryHandlers");

DECLARE_ASYNC_QUERY_HANDLER(L"nodes", NodeQueryHandler);
DECLARE_SYNC_QUERY_HANDLER(L"health", HealthQueryHandler);
DECLARE_ASYNC_QUERY_HANDLER(L"deployedreplicadetail", DeployedReplicaDetailQueryHandler);

bool IQueryHandler::Verify(ComPointer<IFabricQueryClient7> const & queryClient, wstring const & queryName)
{
    bool canRetry = false;
    bool result;

    for (int i = 0; i < 5; i++)
    {
        result = Verify(queryClient, queryName, canRetry);
        if (result || !canRetry)
        {
            return result;
        }
    }

    return result;
}

HRESULT IQueryHandler::BeginVerify(
    ComPointer<IFabricQueryClient7> const &,
    TimeSpan,
    IFabricAsyncOperationCallback *,
    IFabricAsyncOperationContext **)
{
    Assert::CodingError("Not implemented");
}

HRESULT IQueryHandler::EndVerify(
    Common::ComPointer<IFabricQueryClient7> const &,
    IFabricAsyncOperationContext *,
    __out bool &)
{
    Assert::CodingError("Not implemented");
}

bool IQueryHandler::Verify(ComPointer<IFabricQueryClient7> const & queryClient, wstring const & queryName, __out bool & canRetry)
{
    HRESULT hr = TestFabricClient::PerformFabricClientOperation(
        L"Query." + queryName,
        FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
        [&] (DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return BeginVerify(queryClient, TimeSpan::FromMilliseconds(timeout), callback.GetRawPointer(), context.InitializationAddress());
    },
        [&] (ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
    {
        return EndVerify(queryClient, context.GetRawPointer(), canRetry);
    },
        S_OK,
        S_OK,
        S_OK,
        false);

    if (SUCCEEDED(hr))
    {
        TestSession::WriteNoise(TraceSource, "Query succeeded.");
        return true;
    }
    else
    {
        TestSession::WriteError(TraceSource, "Query returned {0}", hr);
        return false;
    }
}

HRESULT NodeQueryHandler::BeginVerify(
    ComPointer<IFabricQueryClient7> const & queryClient,
    TimeSpan timeout,
    IFabricAsyncOperationCallback *callback,
    IFabricAsyncOperationContext **context)
{
    FABRIC_NODE_QUERY_DESCRIPTION queryDesc = {0};
    return queryClient->BeginGetNodeList(&queryDesc, static_cast<DWORD>(timeout.TotalPositiveMilliseconds()), callback, context);
}

HRESULT NodeQueryHandler::EndVerify(
    ComPointer<IFabricQueryClient7> const & queryClient,
    IFabricAsyncOperationContext *context,
    __out bool & canRetry)
{
    canRetry = false;

    ComPointer<IFabricGetNodeListResult2> getNodeListResult;
    HRESULT hr = queryClient->EndGetNodeList2(context, getNodeListResult.InitializationAddress());
    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    FABRIC_PAGING_STATUS const * pagingStatus = getNodeListResult->get_PagingStatus();
    TestSession::FailTestIf(pagingStatus != NULL, "verify should get all results, no paging status");

    FABRIC_NODE_QUERY_RESULT_LIST const * nodeList = getNodeListResult->get_NodeList();
    ULONG count = nodeList->Count;
    FABRIC_NODE_QUERY_RESULT_ITEM const * publicNodeQueryResult = nodeList->Items;
        
    for (ULONG i = 0; i < count; ++i)
    {
        auto fm = FABRICSESSION.FabricDispatcher.GetFM();
        TestSession::FailTestIfNot(fm != nullptr, "FM is not ready");

        NodeId nodeId;
        ErrorCode error = NodeIdGenerator::GenerateFromString(publicNodeQueryResult[i].NodeName, nodeId);
        if (!error.IsSuccess())
        {
            TestSession::WriteInfo(
                TraceSource, 
                "Failed to generate NodeId from NodeName {0}",
                publicNodeQueryResult[i].NodeName);
            return E_FAIL;
        }

        auto nodeInfo = fm->FindNodeByNodeId(nodeId);
        if (!nodeInfo)
        {
            TestSession::WriteInfo(
                TraceSource, 
                "Node {0} not found at FM",
                publicNodeQueryResult[i].NodeName);
            return E_FAIL;
        }

        bool result = StringUtility::Compare(wstring(publicNodeQueryResult[i].NodeName), nodeInfo->NodeName) == 0;
        if(!result) return E_FAIL;

        result = StringUtility::Compare(wstring(publicNodeQueryResult[i].IpAddressOrFQDN), nodeInfo->IpAddressOrFQDN) == 0;
        if(!result) return E_FAIL;

        result = StringUtility::Compare(wstring(publicNodeQueryResult[i].NodeType), nodeInfo->NodeType) == 0;
        if(!result) return E_FAIL;

        result = publicNodeQueryResult[i].NodeStatus == nodeInfo->NodeStatus;
        if(!result) return E_FAIL;

        auto votes = FederationConfig::GetConfig().Votes;
        auto it = votes.find(NodeId(nodeId));
        bool isSeedNode =  (it != votes.end() && it->Type == Federation::Constants::SeedNodeVoteType);

        result = publicNodeQueryResult[i].IsSeedNode == static_cast<BOOLEAN>(isSeedNode);
        if(!result) return E_FAIL;
        
        result = StringUtility::Compare(wstring(publicNodeQueryResult[i].UpgradeDomain), nodeInfo->ActualUpgradeDomainId) == 0;
        if(!result) return E_FAIL;

        TestSession::FailTestIfNot(nodeInfo->FaultDomainIds.size() <= 1, "nodeInfo->Description.NodeFaultDomainIds vector size should always be 1 or 0");
        result = StringUtility::Compare(
            wstring(publicNodeQueryResult[i].FaultDomain), 
            nodeInfo->FaultDomainIds.size() == 0 ? L"" : wformatString(nodeInfo->FaultDomainIds[0])) == 0;
        if(!result) return E_FAIL;

        result = StringUtility::Compare(wstring(publicNodeQueryResult[i].CodeVersion), wformatString(nodeInfo->VersionInstance.Version.CodeVersion)) == 0;
        if(!result) return E_FAIL;

        result = StringUtility::Compare(wstring(publicNodeQueryResult[i].ConfigVersion), wformatString(nodeInfo->VersionInstance.Version.ConfigVersion)) == 0;
        if(!result) return E_FAIL;

        // TODO compare health state
    }
    
    return S_OK;
}

bool HealthQueryHandler::Verify(ComPointer<IFabricQueryClient7> const & queryClient, wstring const & queryName, __out bool & canRetry)
{
    UNREFERENCED_PARAMETER(queryClient);
    UNREFERENCED_PARAMETER(queryName);
    canRetry = true;

    return FABRICSESSION.FabricDispatcher.FabricClientHealth.QueryHealth(StringCollection());
}

HRESULT DeployedReplicaDetailQueryHandler::BeginVerify(
    ComPointer<IFabricQueryClient7> const & queryClient,
    TimeSpan timeout,
    IFabricAsyncOperationCallback *callback,
    IFabricAsyncOperationContext **context)
{
    ScopedHeap heap;
    FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_DESCRIPTION queryDesc = { 0 };
    bool issueQuery = false;

    // Choose a replica from a random service to issue the query 
    {
        auto fm = FABRICSESSION.FabricDispatcher.GetFM();
        TestSession::FailTestIfNot(fm != nullptr, "FM should be valid while running DeployedReplicaDetailQueryHandler");
        auto GFUM = fm->SnapshotGfum();
        if (!GFUM.empty())
        {
            Random rnd;
            auto randomFT = GFUM[rnd.Next() % GFUM.size()];
            auto replicas = randomFT.GetReplicas();

            if (!replicas.empty())
            {
                issueQuery = true;
                auto randomReplica = replicas[rnd.Next() % replicas.size()];

                queryDesc.NodeName = heap.AddString(randomReplica.GetNode().NodeName);
                queryDesc.PartitionId = randomFT.FailoverUnitDescription.FailoverUnitId.Guid.AsGUID();
                queryDesc.ReplicaId = randomReplica.ReplicaId;
                queryDesc.Reserved = NULL;
            }
        }
    }
        
    if (!issueQuery)
    {
        ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
        HRESULT hr = operation->Initialize(S_OK, ComponentRootSPtr(), callback);
        TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
        return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
    }
    else
    {
        TestSession::WriteNoise(
            TraceSource,
            "Issuing GetReplicaDetail query with {0}, {1}, {2} parameters",
            queryDesc.NodeName, Guid(queryDesc.PartitionId), queryDesc.ReplicaId);

        return queryClient->BeginGetDeployedReplicaDetail(&queryDesc, static_cast<DWORD>(timeout.TotalPositiveMilliseconds()), callback, context);
    }
}

HRESULT DeployedReplicaDetailQueryHandler::EndVerify(
    ComPointer<IFabricQueryClient7> const & queryClient,
    IFabricAsyncOperationContext *context,
    __out bool & canRetry)
{
    canRetry = false;
    HRESULT hr = S_OK;
    ComPointer<IFabricGetDeployedServiceReplicaDetailResult> result;
    auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);

    if(castedContext != nullptr)
    {
        return ComCompletedAsyncOperationContext::End(context);
    }
    else
    {
        hr = queryClient->EndGetDeployedReplicaDetail(context, result.InitializationAddress());
    }

    if (SUCCEEDED(hr))
    {
        // There is nothing much to verify in the query result as it is an instantaneous snapshot of the replica.
        auto details = result->get_ReplicaDetail();
        if (details->Kind == FABRIC_SERVICE_KIND_STATEFUL)
        {
            FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM* item = (FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM*)details->Value;
            
            if (item->ReplicatorStatus != nullptr)
            {
                TestSession::FailTestIf(item->ReplicatorStatus->Value == nullptr, "Empty Value in replicator query");

                if (item->ReplicatorStatus->Role == FABRIC_REPLICA_ROLE_PRIMARY)
                {
                    FABRIC_PRIMARY_REPLICATOR_STATUS_QUERY_RESULT * primaryResult = (FABRIC_PRIMARY_REPLICATOR_STATUS_QUERY_RESULT*)item->ReplicatorStatus->Value;

                    TestSession::FailTestIf(primaryResult->ReplicationQueueStatus == nullptr, "Empty Primary Queue Status in replicator query");
                    TestSession::FailTestIf(primaryResult->RemoteReplicators == nullptr, "Empty Remote Replicators in replicator query");
                    TestSession::FailTestIf(primaryResult->ReplicationQueueStatus->QueueUtilizationPercentage > 100, "Primary QueueUtilizationPercentage cannot exceed 100 in replicator query");

                    FABRIC_REMOTE_REPLICATOR_STATUS_LIST * remoteReplicatorStatusList = reinterpret_cast<FABRIC_REMOTE_REPLICATOR_STATUS_LIST*>(primaryResult->RemoteReplicators);

                    for (size_t i = 0; i < remoteReplicatorStatusList->Count; i++)
                    {
                        auto remoteReplicator = static_cast<FABRIC_REMOTE_REPLICATOR_STATUS>(remoteReplicatorStatusList->Items[i]);

                        TestSession::FailTestIf(remoteReplicator.LastReceivedReplicationSequenceNumber < -1, "LastReceivedReplicationSequenceNumber must be greater than -1");
                        TestSession::FailTestIf(remoteReplicator.LastReceivedCopySequenceNumber < 0, "LastReceivedCopySequenceNumber must be greater than zero");

                        TestSession::FailTestIf(remoteReplicator.Reserved == nullptr, "remoteReplicator.Reserved must not be null");
                        auto remoteReplicatorAcknowledgementStatus = static_cast<FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_STATUS*>(remoteReplicator.Reserved);

                        TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->ReplicationStreamAcknowledgementDetails->NotReceivedCount < 0, "NotReceivedReplicationCount must be greater than zero");
                        TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->ReplicationStreamAcknowledgementDetails->ReceivedAndNotAppliedCount < 0, "ReceivedAndNotAppliedReplicationCount must be greater than zero");
                        TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->ReplicationStreamAcknowledgementDetails->AverageApplyDurationMilliseconds < 0, "AverageReplicationApplyAckDurationMilliseconds must be greater than zero");
                        TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->ReplicationStreamAcknowledgementDetails->AverageReceiveDurationMilliseconds < 0, "AverageReplicatoinReceiveAckDurationMilliseconds must be greater than zero");

                        if (remoteReplicator.IsInBuild == TRUE)
                        {
                            TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->CopyStreamAcknowledgementDetails->NotReceivedCount < 0, "NotReceivedCopyCount must be greater than zero");
                            TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->CopyStreamAcknowledgementDetails->ReceivedAndNotAppliedCount < 0, "ReceivedAndNotAppliedCopyCount must be greater than zero");
                            TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->CopyStreamAcknowledgementDetails->AverageApplyDurationMilliseconds < 0, "AverageCopyApplyAckDurationMilliseconds must be greater than zero");
                            TestSession::FailTestIf(remoteReplicatorAcknowledgementStatus->CopyStreamAcknowledgementDetails->AverageReceiveDurationMilliseconds < 0, "AverageCopyReceiveAckDurationMilliseconds must be greater than zero");
                        }
                    }
                }
                else if (item->ReplicatorStatus->Role == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY || item->ReplicatorStatus->Role == FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
                {
                    FABRIC_SECONDARY_REPLICATOR_STATUS_QUERY_RESULT * secondaryResult = (FABRIC_SECONDARY_REPLICATOR_STATUS_QUERY_RESULT*)item->ReplicatorStatus->Value;

                    TestSession::FailTestIf(secondaryResult->ReplicationQueueStatus == nullptr, "Empty Secondary Replication Queue Status in replicator query");
                    TestSession::FailTestIf(secondaryResult->CopyQueueStatus == nullptr, "Empty Secondary Copy Queue Status in replicator query");
                    TestSession::FailTestIf(secondaryResult->IsInBuild && item->ReplicatorStatus->Role == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, "IsInBuild cannot be true when role is active secondary in replicator query");
                    TestSession::FailTestIf(!secondaryResult->IsInBuild && item->ReplicatorStatus->Role == FABRIC_REPLICA_ROLE_IDLE_SECONDARY, "IsInBuild should be true when role is idle secondary in replicator query");
                    TestSession::FailTestIf(secondaryResult->ReplicationQueueStatus->QueueUtilizationPercentage > 100, "Secondary Replication QueueUtilizationPercentage cannot exceed 100 in replicator query");

                    TestSession::FailTestIf(secondaryResult->CopyQueueStatus->QueueUtilizationPercentage > 100, "Secondary Copy QueueUtilizationPercentage cannot exceed 100");
                }
                else
                {
                    TestSession::FailTest("Unknown role in replicator query result - {0}", item->ReplicatorStatus->Role);
                }
            }
        }
    }

    // Ignore error codes from query as replica could be closing/not opened yet.
    return S_OK;
}


