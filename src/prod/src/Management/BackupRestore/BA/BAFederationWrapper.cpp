// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

using namespace Common;
using namespace Transport;
using namespace Federation;
using namespace Reliability;
using namespace Management;
using namespace BackupRestoreAgentComponent;

StringLiteral const TraceSend("Send");

BAFederationWrapper::BAFederationWrapper(Federation::FederationSubsystem & federation, Reliability::ServiceResolver & resolver, Api::IQueryClientPtr queryClientPtr)
    : RootedObject(federation.Root),
    federation_(federation),
    serviceResolver_(resolver),
    queryClientPtr_(queryClientPtr),
    traceId_(federation.IdString),
    instanceIdStr_(federation_.Instance.ToString())
{
}

void BAFederationWrapper::SendToNode(MessageUPtr && message, NodeInstance const & target)
{
    auto action = message->Action;

    federation_.BeginRoute(
        std::move(message),
        target.Id,
        target.InstanceId,
        true,
        TimeSpan::MaxValue,
        TimeSpan::MaxValue,
        [this, action, target] (AsyncOperationSPtr const& routeOperation)
        {
            this->RouteCallback(routeOperation, action, target);
        },
        Root.CreateAsyncOperationRoot());
}

void BAFederationWrapper::RouteCallback(
    AsyncOperationSPtr const& routeOperation,
    wstring const & action,
    NodeInstance const & target)
{
    ErrorCode error = federation_.EndRoute(routeOperation);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceSend,
            traceId_,
            "failed to send message {0} to node {1} with error {2}",
            action,
            target,
            error);
    }
}

AsyncOperationSPtr BAFederationWrapper::BeginRequestToNode(
    MessageUPtr && message,
    NodeInstance const & target,
    bool useExactRouting,
    TimeSpan const retryInterval,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) const
{
    message->Headers.Replace(TimeoutHeader(timeout));

    return federation_.BeginRouteRequest(
        std::move(message),
        target.Id,
        target.InstanceId,
        useExactRouting,
        retryInterval,
        timeout,
        callback,
        parent);
}

ErrorCode BAFederationWrapper::EndRequestToNode(AsyncOperationSPtr const & operation, MessageUPtr & reply) const
{
    return federation_.EndRouteRequest(operation, reply);
}

void BAFederationWrapper::RegisterFederationSubsystemEvents(OneWayMessageHandler const & oneWayMessageHandler, RequestMessageHandler const & requestMessageHandler)
{
    federation_.RegisterMessageHandler(
        Actor::BA,
        oneWayMessageHandler,
        requestMessageHandler,
        false /*dispatchOnTransportThread*/);
}

AsyncOperationSPtr BAFederationWrapper::BeginSendToServiceNode(
    MessageUPtr && message,
    IpcReceiverContextUPtr && receiverContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<NodeToNodeAsyncOperation>(
        std::move(message),
        std::move(receiverContext),
        serviceResolver_,
        federation_,
        queryClientPtr_,
        timeout,
        callback,
        parent);
}

ErrorCode BAFederationWrapper::EndSendToServiceNode(AsyncOperationSPtr const & operation, MessageUPtr & reply, IpcReceiverContextUPtr & receiverContext) const
{
    return NodeToNodeAsyncOperation::End(operation, reply, receiverContext);
}
