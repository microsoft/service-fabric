// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Query;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ReconfigurationAgentComponent::Query;
using namespace Infrastructure;
using namespace ServiceModel;

HostQueryAsyncOperation::HostQueryAsyncOperation(
    ReconfigurationAgent & ra,
    wstring activityId,
    Infrastructure::EntityEntryBaseSPtr const & entityEntry,
    wstring const & runtimeId,
    FailoverUnitDescription const & failoverUnitDescription,
    ReplicaDescription const & replicaDescription,
    wstring const & hostId,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
    : AsyncOperation(callback, parent)
    , ra_(ra)
    , activityId_(activityId)
    , entityEntry_(entityEntry)
    , runtimeId_(runtimeId)
    , failoverUnitDescription_(failoverUnitDescription)
    , replicaDescription_(replicaDescription)
    , hostId_(hostId)
    , replicaDetailQueryResult_()
{
}

HostQueryAsyncOperation::~HostQueryAsyncOperation()
{
}

void HostQueryAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    ProxyRequestMessageBody body(
        runtimeId_,
        failoverUnitDescription_,
        replicaDescription_,
        ProxyMessageFlags::None);

    auto message = RAMessage::GetRAPQuery().CreateMessage(body);
    auto failoverUnitEntry = static_pointer_cast<EntityEntry<FailoverUnit>>(entityEntry_);
    failoverUnitEntry->AddEntityIdToMessage(*message);

    auto operation = ra_.RAPTransport.BeginRequest(
        move(message),
        hostId_,
        ra_.Config.RAPMessageRetryInterval,
        [this](AsyncOperationSPtr const & inner)
        {
            if (!inner->CompletedSynchronously)
            {
                OnRequestReplyCompleted(inner);
            }
        },
        thisSPtr);

    if (operation->CompletedSynchronously)
    {
        OnRequestReplyCompleted(operation);
    }
}

void HostQueryAsyncOperation::OnRequestReplyCompleted(
    AsyncOperationSPtr const & operation)
{
    auto error = ProcessReply(operation);
    TryComplete(operation->Parent, error);
}

ErrorCode HostQueryAsyncOperation::ProcessReply(
    AsyncOperationSPtr const & operation)
{
    Transport::MessageUPtr reply;
    ErrorCode error = ra_.RAPTransport.EndRequest(operation, reply);
    if (!error.IsSuccess())
    {
        ReconfigurationAgent::WriteNoise("Query", "EndRequest failed with {0}. Activity {1}", error, activityId_);
        return error;
    }

    if (reply == nullptr)
    {
        Assert::TestAssert("Should have valid reply if success {0}", activityId_);
        ReconfigurationAgent::WriteNoise("Query", "Reply null Activity {0}", activityId_);
        return ErrorCodeValue::OperationFailed;
    }

    if (reply->Action != RAMessage::GetRAPQueryReply().Action)
    {
        Assert::TestAssert("Unknown reply action {0} for {1}", reply->Action, activityId_);
        ReconfigurationAgent::WriteNoise("Query", "Unknown reply action {0} Activity {1}", reply->Action, activityId_);
        return ErrorCodeValue::OperationFailed;
    }

    ProxyQueryReplyMessageBody<DeployedServiceReplicaDetailQueryResult> body;
    if (!reply->GetBody(body))
    {
        ReconfigurationAgent::WriteNoise("Query", "Received malformed reply {0}", reply->MessageId);
        return ErrorCodeValue::OperationFailed;
    }

    ReconfigurationAgent::WriteNoise("Query", "RA on node {0} processing query reply {1} {2}", ra_.NodeInstanceIdStr, body.ErrorCode, body.QueryResult);
    if (body.ErrorCode.IsSuccess())
    {
        RAEventSource::Events->DeployedReplicaDetailQueryResponse(ra_.NodeInstanceIdStr, activityId_, wformatString(body.QueryResult));
        replicaDetailQueryResult_ = static_cast<ServiceModel::DeployedServiceReplicaDetailQueryResult>(body.QueryResult);
        return ErrorCodeValue::Success;
    }

    return body.ErrorCode;
}

ServiceModel::DeployedServiceReplicaDetailQueryResult HostQueryAsyncOperation::GetReplicaDetailQueryResult()
{
    return replicaDetailQueryResult_;
}
