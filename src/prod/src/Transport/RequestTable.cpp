// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;

static Common::StringLiteral const TraceType("RequestTable");

RequestTable::RequestTable() : table_()
{
}

ErrorCode RequestTable::TryInsertEntry(Transport::MessageId const & messageId, RequestAsyncOperationSPtr & entry)
{
    auto error = this->table_.TryAdd(messageId, entry);
    if (!error.IsSuccess())
    {
        if (error.IsError(ErrorCodeValue::ObjectClosed))
        {
            WriteInfo(TraceType, "TryInsertEntry: dropping message {0}, failed to insert: {1}", messageId, error);
        }
        else
        {
            WriteError(TraceType, "TryInsertEntry: dropping message {0}, failed to insert: {1}", messageId, error);
            Assert::TestAssert();
        }
    }

    return error;
}

bool RequestTable::TryRemoveEntry(Transport::MessageId const & messageId, RequestAsyncOperationSPtr & operation)
{
    return this->table_.TryGetAndRemove(messageId, operation);
}

std::vector<std::pair<Transport::MessageId, RequestAsyncOperationSPtr>> RequestTable::RemoveIf(std::function<bool(std::pair<MessageId, RequestAsyncOperationSPtr> const&)> const & predicate)
{
    return table_.RemoveIf(predicate);
}

bool RequestTable::OnReplyMessage(Transport::Message & reply)
{
    // If this method fails to remove from the table or complete the request, that means it has already been completed (most likely with a timeout)
    RequestAsyncOperationSPtr operation;
    if (!this->TryRemoveEntry(reply.RelatesTo, operation))
    {
        return false;
    }

    // Create an error with the fault value of the message and complete it.  This will bubble up the fault code.
    ErrorCode error(reply.FaultErrorCodeValue);
    if (error.IsSuccess())
    {
        operation->SetReply(reply.Clone()); // todo, leikong, consider avoiding unnecessary cloning
    }
    else if (reply.HasFaultBody)
    {
        RejectFaultBody body;
        if (reply.GetBody(body) && body.HasErrorMessage())
        {
            error.Overwrite(body.TakeError());
        }
    }

    return operation->TryComplete(operation, error);
}

void RequestTable::Close()
{
    auto tableCopy = this->table_.Close();
    for(auto iter = tableCopy.begin(); iter != tableCopy.end(); ++ iter)
    {
        iter->second->Cancel();
    }
}
