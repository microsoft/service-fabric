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

QueryUtility::QueryUtility()
{
}

vector<EntityEntryBaseSPtr> QueryUtility::GetAllEntityEntries(
    ReconfigurationAgent & ra) const
{
    return ra.LocalFailoverUnitMapObj.GetAllFailoverUnitEntries(false);
}

vector<EntityEntryBaseSPtr> QueryUtility::GetEntityEntriesByFailoverUnitId(
    ReconfigurationAgent & ra,
    FailoverUnitId const & ftId) const
{
    vector<EntityEntryBaseSPtr> entities;
    auto entry = ra.LocalFailoverUnitMapObj.GetEntry(ftId);

    if (entry != nullptr)
    {
        entities.push_back(move(entry));
    }

    return entities;
}

void QueryUtility::SendErrorResponseForQuery(
    ReconfigurationAgent & ra,
    wstring const & activityId,
    Common::ErrorCode const & error,
    Federation::RequestReceiverContext & context)
{
    QueryResult result(error);
    auto msg = RSMessage::GetQueryReply().CreateMessage(result);
    ra.Federation.CompleteRequestReceiverContext(context, (move(msg)));

    RAEventSource::Events->QueryResponse(ra.NodeInstanceIdStr, activityId, error, 0);
}

void QueryUtility::SendResponseForGetServiceReplicaListDeployedOnNodeQuery(
    ReconfigurationAgent & ra,
    wstring const & activityId,
    vector<DeployedServiceReplicaQueryResult> && items,
    RequestReceiverContext & context)
{
    auto count = items.size();
    QueryResult result(move(items));
    auto message = RSMessage::GetQueryReply().CreateMessage(result);
    ra.Federation.CompleteRequestReceiverContext(context, (move(message)));

    RAEventSource::Events->QueryResponse(ra.NodeInstanceIdStr, activityId, ErrorCodeValue::Success, static_cast<uint64>(count));
}

void QueryUtility::SendResponseForGetDeployedServiceReplicaDetailQuery(
    ReconfigurationAgent & ra,
    DeployedServiceReplicaDetailQueryResult & deployedServiceReplicaDetailQueryResult,
    RequestReceiverContext & context)
{
    auto queryResult = ServiceModel::QueryResult(Common::make_unique<DeployedServiceReplicaDetailQueryResult>(deployedServiceReplicaDetailQueryResult));
    auto message = RSMessage::GetQueryReply().CreateMessage(queryResult);
    ra.Federation.CompleteRequestReceiverContext(context, (move(message)));
}
