// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace Reliability;

    StringLiteral const TraceComponent("ProcessRequest.ResolvePartition");

    EntreeService::ResolvePartitionAsyncOperation::ResolvePartitionAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : AdminRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::ResolvePartitionAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        StartResolve(thisSPtr);
    }

    void EntreeService::ResolvePartitionAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {        
        TimedAsyncOperation::OnStart(thisSPtr);

        StartResolve(thisSPtr);
    }

    void EntreeService::ResolvePartitionAsyncOperation::StartResolve(AsyncOperationSPtr const & thisSPtr)
    {        
        Reliability::ConsistencyUnitId cuid;
        if (ReceivedMessage->GetBody(cuid))
        {
            WriteNoise(
                TraceComponent,
                "{0}: resolving location for partition {1}",
                TraceId,
                cuid);

            Reliability::ServiceTableEntry entry;
            if (cuid == ConsistencyUnitId(ServiceModel::Constants::FMServiceGuid))
            {
                auto inner = Properties.Resolver.BeginResolveFMService(
                    0,
                    GenerationNumber(),
                    CacheMode::BypassCache,
                    ActivityHeader,
                    RemainingTime,
                    [this](AsyncOperationSPtr const & a) { OnFMResolved(a, false /*expectedCompletedSynchronously*/); },
                    thisSPtr);
                OnFMResolved(inner, true /*expectedCompletedSynchronously*/);
            }
            else
            {
                vector<VersionedCuid> partitionIdVector;
                partitionIdVector.push_back(VersionedCuid(cuid));
                auto inner = Properties.Resolver.BeginResolveServicePartition(
                    partitionIdVector,
                    CacheMode::BypassCache,
                    ActivityHeader,
                    RemainingTime,
                    [this](AsyncOperationSPtr const & a) 
                    {
                        OnRequestComplete(a, false /*expectedCompletedSynchronously*/); 
                    },
                    thisSPtr);
                OnRequestComplete(inner, true /*expectedCompletedSynchronously*/);
            }
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::ResolvePartitionAsyncOperation::OnFMResolved(
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        Reliability::ServiceTableEntry serviceTableEntry;
        GenerationNumber unused;
        ErrorCode error = Properties.Resolver.EndResolveFMService(asyncOperation, /*out*/serviceTableEntry, /*out*/unused);

        if (error.IsSuccess())
        {
            Reply = NamingMessage::GetResolvePartitionReply(serviceTableEntry);
        }

        CompleteOrRetry(asyncOperation->Parent, error);
    }

    void EntreeService::ResolvePartitionAsyncOperation::OnRequestComplete(
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

            Reply = NamingMessage::GetResolvePartitionReply(entries[0]);
        }

        CompleteOrRetry(asyncOperation->Parent, error);
    }

    void EntreeService::ResolvePartitionAsyncOperation::CompleteOrRetry(
        AsyncOperationSPtr const & thisSPtr, 
        ErrorCode const & error)
    {
        if (error.IsSuccess() || !this->IsRetryable(error))
        {
            this->TryComplete(thisSPtr, error);
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: resolve returned error {1}, retry (timeout left={2})",
                TraceId,
                error,
                this->RemainingTime);
            this->HandleRetryStart(thisSPtr);
        }
    }
}
