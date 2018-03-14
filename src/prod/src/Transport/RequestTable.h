// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class RequestTable : Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
        DENY_COPY(RequestTable);

    public:
        RequestTable();

        Common::ErrorCode TryInsertEntry(Transport::MessageId const & messageId, RequestAsyncOperationSPtr & entry);

        bool TryRemoveEntry(Transport::MessageId const & messageId, RequestAsyncOperationSPtr & operation);

        std::vector<std::pair<Transport::MessageId, RequestAsyncOperationSPtr>> RemoveIf(std::function<bool(std::pair<MessageId, RequestAsyncOperationSPtr> const&)> const & predicate);

        bool OnReplyMessage(Transport::Message & reply);

        void Close();

    private:
        Common::SynchronizedMap<Transport::MessageId, RequestAsyncOperationSPtr> table_;
    };
}
