// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace Transport;
    using namespace Reliability;

    EntreeService::ResolveNameOwnerAsyncOperation::ResolveNameOwnerAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : AdminRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::ResolveNameOwnerAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {        
        if (this->TryGetNameFromRequestBody())
        {
            TimedAsyncOperation::OnStart(thisSPtr);

            auto cuid = Properties.NamingCuidCollection.HashToNameOwner(Name);

            auto inner = Properties.Resolver.BeginResolveServicePartition(
                cuid,
                ActivityHeader,
                RemainingTime,
                [this](AsyncOperationSPtr const & a) 
                { 
                    OnRequestComplete(a, false /*expectedCompletedSynchronously*/); 
                },
                thisSPtr);
            OnRequestComplete(inner, true /*expectedCompletedSynchronously*/);    
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::ResolveNameOwnerAsyncOperation::OnRequestComplete(
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        std::vector<Reliability::ServiceTableEntry> entries;
        GenerationNumber unused;
        ErrorCode error = Properties.Resolver.EndResolveServicePartition(asyncOperation, /*out*/entries, /*out*/unused);

        if (error.IsSuccess())
        {
            ASSERT_IFNOT(
                entries.size() == static_cast<size_t>(1),
                "entries.size() == static_cast<size_t>(1)");

            this->SetReplyAndComplete(asyncOperation->Parent, NamingMessage::GetResolvePartitionReply(entries[0]), error);
        }
        else
        {
            TryComplete(asyncOperation->Parent, error);
        }
    }
}
