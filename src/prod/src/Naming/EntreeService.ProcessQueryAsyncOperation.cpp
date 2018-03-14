// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Query;
    using namespace Transport;

    StringLiteral const TraceComponent("ProcessQueryAsyncOperation");

    EntreeService::ProcessQueryAsyncOperation::ProcessQueryAsyncOperation(
        __in EntreeService & owner,
        Query::QueryNames::Enum queryName,
        ServiceModel::QueryArgumentMap const & queryArgs,
        ActivityId const & activityId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , queryName_(queryName)
        , queryArgs_(queryArgs)
        , activityId_(activityId)
    {
    }

    ErrorCode EntreeService::ProcessQueryAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation,
        __out MessageUPtr &reply)
    {
        auto casted = AsyncOperation::End<ProcessQueryAsyncOperation>(asyncOperation);
        if (casted->reply_)
        {
            reply = move(casted->reply_);
        }

        return casted->Error;
    }

    void EntreeService::ProcessQueryAsyncOperation::OnStart(
        AsyncOperationSPtr const & thisSPtr)
    {
        switch (queryName_)
        {
        case QueryNames::GetQueries:
            HandleGetQueryListAsync(thisSPtr);
            break;
        case QueryNames::GetApplicationName:
            HandleGetApplicationNameAsync(thisSPtr);
            break;
        default:
            reply_ = Common::make_unique<Transport::Message>(ServiceModel::QueryResult());
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidArgument));
        }
    }

    void EntreeService::ProcessQueryAsyncOperation::HandleGetQueryListAsync(
        AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.BeginGetQueryListOperation(
            this->RemainingTime,
            [this, thisSPtr](AsyncOperationSPtr const & operation){ OnGetQueryListCompleted(operation, false); },
            thisSPtr);
        OnGetQueryListCompleted(operation, true);
    }

    void EntreeService::ProcessQueryAsyncOperation::OnGetQueryListCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {

        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.EndGetQueryListOperation(operation, reply_);
        TryComplete(operation->Parent, error);
    }

    void EntreeService::ProcessQueryAsyncOperation::HandleGetApplicationNameAsync(
        AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.BeginGetApplicationNameOperation(
            queryArgs_,
            activityId_,
            this->RemainingTime,
            [this, thisSPtr](AsyncOperationSPtr const & operation){ OnGetApplicationNameCompleted(operation, false); },
            thisSPtr);
        OnGetApplicationNameCompleted(operation, true);
    }

    void EntreeService::ProcessQueryAsyncOperation::OnGetApplicationNameCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {

        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.EndGetApplicationNameOperation(operation, reply_);
        TryComplete(operation->Parent, error);
    }
}
