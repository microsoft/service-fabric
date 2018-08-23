// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Naming;
using namespace Common;
using namespace Transport;
using namespace Query;
using namespace ServiceModel;
using namespace SystemServices;
using namespace Federation;
using namespace ClientServerTransport;

StringLiteral const TraceComponent("ForwardToTokenValidationService");

void EntreeService::ForwardToTokenValidationServiceAsyncOperation::OnStartRequest(AsyncOperationSPtr const& thisSPtr)
{
    TimedAsyncOperation::OnStart(thisSPtr);

    Transport::ForwardMessageHeader forwardMessageHeader;
    if (!ReceivedMessage->Headers.TryReadFirst(forwardMessageHeader))
    {
        WriteWarning(TraceComponent, "{0} ForwardMessageHeader missing: {1}", this->TraceId, *ReceivedMessage);
        TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        return;
    }

    ServiceRoutingAgentHeader routingAgentHeader;
    bool hasRoutingAgentHeader = ReceivedMessage->Headers.TryReadFirst(routingAgentHeader);

    FabricActivityHeader activityHeader;
    if (!ReceivedMessage->Headers.TryReadFirst(activityHeader))
    {
        activityHeader = FabricActivityHeader(Guid::NewGuid());

        WriteWarning(
            TraceComponent,
            "{0} message {1} missing activity header: generated = {2}",
            this->TraceId,
            *ReceivedMessage,
            activityHeader);
    }

    ReceivedMessage->Headers.RemoveAll();

    ReceivedMessage->Headers.Add(ActorHeader(forwardMessageHeader.Actor));
    ReceivedMessage->Headers.Add(ActionHeader(forwardMessageHeader.Action));
    ReceivedMessage->Headers.Add(activityHeader);

    if (hasRoutingAgentHeader)
    {
        ReceivedMessage->Headers.Add(routingAgentHeader);
    }

    Guid partitionId;
    if (!this->Properties.TryGetTvsCuid(partitionId))
    {
        GetServicePartitionReplicaList(thisSPtr);
    }
    else
    {
        OnForwardToService(thisSPtr, partitionId);
    }
}

void EntreeService::ForwardToTokenValidationServiceAsyncOperation::GetServicePartitionReplicaList(AsyncOperationSPtr const& thisSPtr)
{
    QueryArgumentMap argsMap;
    argsMap.Insert(QueryResourceProperties::Node::Name,
        Federation::NodeIdGenerator::GenerateNodeName(this->Properties.Instance.Id));

    argsMap.Insert(QueryResourceProperties::Application::ApplicationName, SystemServiceApplicationNameHelper::SystemServiceApplicationName);

    auto body = Common::make_unique<QueryRequestMessageBody>(
        QueryNames::ToString(QueryNames::GetServiceReplicaListDeployedOnNode),
        argsMap);

    MessageUPtr queryMessage = QueryTcpMessage::GetQueryRequest(std::move(body))->GetTcpMessage();
    AsyncOperation::CreateAndStart<QueryRequestOperation>(
        this->Properties,
        move(queryMessage),
        RemainingTime,
        [this](AsyncOperationSPtr const& operation) { this->OnGetServicePartitionReplicaListCompleted(operation); },
        thisSPtr);
}

void EntreeService::ForwardToTokenValidationServiceAsyncOperation::OnGetServicePartitionReplicaListCompleted(AsyncOperationSPtr const & operation)
{
    MessageUPtr reply;
    auto error = QueryRequestOperation::End(operation, reply);
    if (!error.IsSuccess())
    {
        TryComplete(operation->Parent, error);
        return;
    }

    if (reply.get() == nullptr)
    {
        TRACE_WARNING_AND_TESTASSERT(TraceComponent, "{0}: null reply message on success", this->TraceId);

        TryComplete(operation->Parent, ErrorCodeValue::OperationFailed);
        return;
    }

    QueryResult result;
    error = ErrorCodeValue::OperationFailed;
    if (reply->GetBody(result))
    {
        error.ReadValue();
        error = result.QueryProcessingError;
    }

    if (!error.IsSuccess())
    {
        TryComplete(operation->Parent, error);
        return;
    }

    vector<DeployedServiceReplicaQueryResult> replicaList;
    result.MoveList<DeployedServiceReplicaQueryResult>(replicaList);
    Guid partitionId;
    for(auto iter = replicaList.begin(); iter != replicaList.end(); ++iter)
    {
        if (StringUtility::AreEqualCaseInsensitive((*iter).ServiceTypeName, SystemServiceApplicationNameHelper::TokenValidationServiceType))
        {
            partitionId = (*iter).get_PartitionId();
            break;
        }
    }

    if (partitionId == Guid::Empty())
    {
        WriteWarning(
            TraceComponent, 
            "Failed to get tokenvalidationservice partition Id for service placed on node {0}",
            this->Properties.Instance.Id);
        TryComplete(operation->Parent, ErrorCodeValue::OperationFailed);

        return;
    }

    this->Properties.SetTvsCuid(partitionId);

    OnForwardToService(operation->Parent, partitionId);
}

void EntreeService::ForwardToTokenValidationServiceAsyncOperation::OnComplete(AsyncOperationSPtr const& operation)
{
    MessageUPtr reply;
    auto error = this->Properties.Gateway.EndDispatchGatewayMessage(operation, reply);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent, 
            "Forward to Token Validation Service failed with error {0}",
            error);
        TryComplete(operation->Parent, error);
    }
    else
    {
        this->SetReplyAndComplete(operation->Parent, move(reply), error);
    }    
}

void EntreeService::ForwardToTokenValidationServiceAsyncOperation::OnForwardToService(
    AsyncOperationSPtr const& operation,
    Guid const & partitionId)
{
    SystemServiceLocation location(
        NodeInstance(),
        partitionId,
        SystemServiceLocation::ReplicaId_AnyReplica,
        SystemServiceLocation::ReplicaInstance_AnyReplica);

    ReceivedMessage->Headers.Replace(location.CreateFilterHeader());
    ReceivedMessage->Headers.Replace(TimeoutHeader(RemainingTime));

    FabricActivityHeader activityHeader;
    if (!ReceivedMessage->Headers.TryReadFirst(activityHeader))
    {
        activityHeader = FabricActivityHeader(Guid::NewGuid());
        ReceivedMessage->Headers.Replace(activityHeader);
    }

    //
    // Messages generated at FabricClient have the actual actor/action in the forward header.
    //
    ForwardMessageHeader forwardHeader;
    if (ReceivedMessage->Headers.TryReadFirst(forwardHeader))
    {
        ReceivedMessage->Headers.Replace(ActionHeader(forwardHeader.Action));
        ReceivedMessage->Headers.Replace(ActorHeader(forwardHeader.Actor));
    }

    this->Properties.Gateway.BeginDispatchGatewayMessage(
        move(ReceivedMessage->Clone()),
        RemainingTime,
        [this](AsyncOperationSPtr const& operation) { this->OnComplete(operation); },
        operation);
}
