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

    StringLiteral const TraceComponent("ProcessRequest.ServiceResolution");

    EntreeService::ServiceResolutionAsyncOperationBase::ServiceResolutionAsyncOperationBase(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : NamingRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
      , requestBody_()
      , psd_()
    {
    }

    void EntreeService::ServiceResolutionAsyncOperationBase::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->ReceivedMessage->GetBody(requestBody_))
        {
            TimedAsyncOperation::OnStart(thisSPtr);

            this->Name = requestBody_.Name;

            this->GetPsd(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCode::FromNtStatus(this->ReceivedMessage->GetStatus()));
        }
    }

    ErrorCode EntreeService::ServiceResolutionAsyncOperationBase::GetCurrentUserCuidToResolve(
        ServiceResolutionRequestData const & request,
        __out VersionedCuid & previous)
    {
        ConsistencyUnitId cuid;
        if (psd_->TryGetCuid(request.Key, cuid))
        {
            previous = VersionedCuid(cuid, request.Version.FMVersion, request.Version.Generation);

            return ErrorCodeValue::Success;
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "{0}: Invalid partition key: service = {1} key = {2}", 
                this->TraceId,
                Name.Name,
                request.Key);

            return ErrorCodeValue::InvalidServicePartition;
        }
    }

    void EntreeService::ServiceResolutionAsyncOperationBase::ResolveLocations(
        AsyncOperationSPtr const & thisSPtr,
        PartitionedServiceDescriptorSPtr const & psd)
    {
        psd_ = psd;

        VersionedCuid previous;
        auto error = this->GetCurrentUserCuidToResolve(requestBody_.Request, previous);
        if (!error.IsSuccess())
        {            
            this->TryComplete(thisSPtr, error);

            return;
        }

        // Determine if we need to do a PSD lookup if the service location's version is current
        //
        ServiceTableEntry cachedLocation;
        GenerationNumber cachedGeneration;
        bool foundInCache = Properties.Resolver.TryGetCachedEntry(previous.Cuid, cachedLocation, cachedGeneration);

        if (foundInCache)
        {
            this->Properties.Trace.ResolveFromCache(
                TraceId,
                previous.Cuid.Guid,
                previous.Version,
                previous.Generation,
                cachedLocation.Version,
                cachedGeneration);

            foundInCache = 
                (previous.Generation < cachedGeneration) ||
                (previous.Generation == cachedGeneration && previous.Version < cachedLocation.Version) ||
                //
                // Previous resolution result was from a different partition
                // (e.g. service deleted/re-created or prefix match changed),
                // so accept cached endpoints.
                //
                (!requestBody_.Request.Cuid.IsInvalid && requestBody_.Request.Cuid != previous.Cuid);
        }
        else
        {
            this->Properties.Trace.ResolveFromFM(
                TraceId,
                previous.Cuid.Guid,
                previous.Version,
                previous.Generation);

            foundInCache = false;
        }

        if (foundInCache)
        {
            SendReplyAndComplete(thisSPtr, cachedLocation, cachedGeneration);
        }
        else
        {
            vector<VersionedCuid> partitions;
            partitions.push_back(previous);
            auto inner = Properties.Resolver.BeginResolveServicePartition(
                partitions,
                Reliability::CacheMode::Refresh,
                ActivityHeader,
                RemainingTime,
                [this](AsyncOperationSPtr const & asyncOperation)
                {
                    OnResolveUserServicesComplete(asyncOperation, false /*expectedCompletedSynchronously*/);
                },
                thisSPtr);
            OnResolveUserServicesComplete(inner, true /*expectedCompletedSynchronously*/);
        }
    }

    void EntreeService::ServiceResolutionAsyncOperationBase::OnResolveUserServicesComplete(
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        vector<ServiceTableEntry> entries;
        GenerationNumber generation;
        ErrorCode error = Properties.Resolver.EndResolveServicePartition(asyncOperation, entries, generation);
        if (error.IsSuccess())
        {
            ASSERT_IFNOT(
                entries.size() == 1,
                "{0}: Cannot have success and no more or less than one service table entry during a resolution. name = {1} size = {2}",
                TraceId,
                Name,
                entries.size());
            SendReplyAndComplete(asyncOperation->Parent, entries[0], generation);
        }
        else if (this->IsRetryable(error))
        {
            this->Properties.Trace.RetryResolveFromFM(
                this->TraceId,
                Properties.OperationRetryInterval,
                error);

            this->HandleRetryStart(asyncOperation->Parent);
        }
        else
        {
            this->TryComplete(asyncOperation->Parent, error);
        }
    }

    void EntreeService::ServiceResolutionAsyncOperationBase::SendReplyAndComplete(
        AsyncOperationSPtr const & thisSPtr,
        ServiceTableEntry const & cachedLocation,
        GenerationNumber const & generation)
    {
        auto replyMessage = NamingMessage::GetResolveServiceReply(ResolvedServicePartition(
            psd_->IsServiceGroup,
            cachedLocation,
            psd_->GetPartitionInfo(cachedLocation.ConsistencyUnitId),
            generation,
            psd_->Version,
            (requestBody_.Request.IncludePsd ? psd_ : nullptr)));
        this->SetReplyAndComplete(thisSPtr, move(replyMessage), ErrorCodeValue::Success);
    }
}
