// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ServiceModel;

Common::ErrorCode ReconfigurationAgentProxy::ActionListExecutorReplicatorQueryAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation,
    __out FailoverUnitProxyContext<ProxyRequestMessageBody>* & context,
    __out ProxyActionsListTypes::Enum & actionsList,
    __out ServiceModel::ReplicatorStatusQueryResultSPtr & replicatorResult,
    __out ServiceModel::DeployedServiceReplicaDetailQueryResult & fupQueryResult)
{
    auto thisPtr = AsyncOperation::End<ReconfigurationAgentProxy::ActionListExecutorReplicatorQueryAsyncOperation>(asyncOperation);

    context = &(thisPtr->context_);
    actionsList = thisPtr->actionListName_;

    ASSERT_IF(actionsList != ProxyActionsListTypes::ReplicatorGetQuery, "This method can handle only ReplicatorGetQuery action list");

    if (thisPtr->Error.IsSuccess())
    {
        replicatorResult = move(thisPtr->replicatorQueryResult_);
    }

    // Always return the FUP result
    // Validate input arg
    ASSERT_IF(!fupQueryResult.IsInvalid, "Expected an invalid result to be passed in and for this method to populate out param");
    ASSERT_IF(thisPtr->fupQueryResult_.IsInvalid, "Query result being held by this async op cannot be invalid");
    fupQueryResult = move(thisPtr->fupQueryResult_);

    return thisPtr->Error;
}

ProxyErrorCode ReconfigurationAgentProxy::ActionListExecutorReplicatorQueryAsyncOperation::DoReplicatorGetQuery(AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously)
{
    UNREFERENCED_PARAMETER(thisSPtr);

    // Synchronous operation, so always completed synchronously
    isCompletedSynchronously = true;

    ASSERT_IFNOT(context_.FailoverUnitProxy, "FailoverUnitProxy is invalid");

    // Get the replicator query result
    return context_.FailoverUnitProxy->ReplicatorGetQuery(replicatorQueryResult_);
}

