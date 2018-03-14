// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Federation;
    using namespace Reliability;
    using namespace SystemServices;
    using namespace Transport;
    
    StringLiteral const TraceComponent("ProcessRequest.NamingRequest");

    EntreeService::NamingRequestAsyncOperationBase::NamingRequestAsyncOperationBase(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : RequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
      , lastNamingPartitionLocationVersionUsed_()
    {
    }

    EntreeService::NamingRequestAsyncOperationBase::NamingRequestAsyncOperationBase(
        __in GatewayProperties & properties,
        NamingUri const & name,
        FabricActivityHeader const & activityHeader,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : RequestAsyncOperationBase (properties, name, activityHeader, timeout, callback, parent)
      , lastNamingPartitionLocationVersionUsed_()
    {
    }

    void EntreeService::NamingRequestAsyncOperationBase::StartStoreCommunication(AsyncOperationSPtr const & thisSPtr, NamingUri const & toResolve)
    {
        if (toResolve == NamingUri::RootNamingUri)
        {
            // Authorities are distributed around the federation so we 
            // don't support enumeration at the root
            //
            this->RaiseResolutionErrorAndComplete(thisSPtr, ErrorCodeValue::AccessDenied);
            return;
        }

        Properties.Trace.StartStoreCommunication(this->TraceId, toResolve);

        auto operation = AsyncOperation::CreateAndStart<ResolveNameLocationAsyncOperation>(
            Properties.Resolver,
            toResolve,
            IsRetrying ? CacheMode::Refresh : CacheMode::UseCached,
            lastNamingPartitionLocationVersionUsed_,
            Properties.NamingCuidCollection,
            this->BaseTraceId,
            this->ActivityHeader,
            RemainingTime,
            [this](AsyncOperationSPtr const & operation) 
            {
                this->OnResolveNamingServiceComplete(operation, false /*expectedCompletedSynchronously*/); 
            },
            thisSPtr);

        this->OnResolveNamingServiceComplete(operation, true /*expectedCompletedSynchronously*/);
    }

    void EntreeService::NamingRequestAsyncOperationBase::OnResolveNamingServiceComplete(
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = asyncOperation->Parent;

        SystemServiceLocation serviceLocation;
        ErrorCode error = ResolveNameLocationAsyncOperation::End(asyncOperation, serviceLocation, lastNamingPartitionLocationVersionUsed_);

        if (error.IsSuccess())
        {
            MessageUPtr newMessage = CreateMessageForStoreService();
            if (!newMessage)
            {
                this->RaiseResolutionErrorAndComplete(thisSPtr, ErrorCodeValue::UnsupportedNamingOperation);
                return;
            }        

            WriteNoise(
                TraceComponent,
                "{0}: routing Naming Store Service request to [{1}]",
                this->TraceId,
                serviceLocation);

            auto instance = this->Properties.RequestInstance.GetNextInstance();

            newMessage->Headers.Replace(ActivityHeader);
            newMessage->Headers.Replace(TimeoutHeader(RemainingTime));
            newMessage->Headers.Replace(serviceLocation.CreateFilterHeader());
            newMessage->Headers.Replace(RequestInstanceHeader(instance));

            this->RouteToNode(
                std::move(newMessage),
                serviceLocation.NodeInstance.Id,
                serviceLocation.NodeInstance.InstanceId,
                true,
                thisSPtr);
        }
        else if (this->IsRetryable(error))
        {
            WriteInfo(
                TraceComponent,
                "{0}: could not resolve Naming Store Service. Retrying in {1} seconds. error = {2}",
                this->TraceId,
                Properties.OperationRetryInterval,
                error);

            this->OnResolveNamingServiceRetryableError(thisSPtr, error);

            this->HandleRetryStart(thisSPtr);
        }
        else
        {
            this->RaiseResolutionErrorAndComplete(thisSPtr, error);
        }
    }    

    void EntreeService::NamingRequestAsyncOperationBase::OnRouteToNodeSuccessful(
            Common::AsyncOperationSPtr const & thisSPtr,
            Transport::MessageUPtr & reply)
    {
        OnStoreCommunicationFinished(thisSPtr, std::move(reply));
    }

    void EntreeService::NamingRequestAsyncOperationBase::OnRouteToNodeFailedRetryableError(
            Common::AsyncOperationSPtr const & thisSPtr,
            Transport::MessageUPtr & reply,
            Common::ErrorCode const & error)
    {
        WriteInfo(
            TraceComponent,
            "{0}: could not route to Naming Store Service. Retrying in {1} seconds. error = {2}",
            this->TraceId,
            Properties.OperationRetryInterval,
            error);

        if (reply != nullptr && reply->Action == NamingMessage::NamingServiceCommunicationFailureReplyAction)
        {
            CacheModeHeader header;
            if (NamingMessage::TryGetNamingHeader(*reply, header))
            {
                if (ReceivedMessage)
                {
                    ReceivedMessage->Headers.Replace(CacheModeHeader(CacheMode::Refresh, header.PreviousVersion));
                }
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: Gateway received message with actor {1} that didn't have a cache mode header with a previous version.",
                    TraceId,
                    reply->Actor);
            }
        }

        this->HandleRetryStart(thisSPtr);
    }

    void EntreeService::NamingRequestAsyncOperationBase::OnRouteToNodeFailedNonRetryableError(
            Common::AsyncOperationSPtr const & thisSPtr,
            Common::ErrorCode const & error)
    {
        if (error.IsError(ErrorCodeValue::NameNotFound))
        {
            OnNameNotFound(thisSPtr);
        }
        else
        {
            TryComplete(thisSPtr, error);
        }       
    }

    void EntreeService::NamingRequestAsyncOperationBase::OnResolveNamingServiceNonRetryableError(
        AsyncOperationSPtr const &,
        ErrorCode const &)
    {
        // no-op unless overridden
    }

    void EntreeService::NamingRequestAsyncOperationBase::OnResolveNamingServiceRetryableError(
        AsyncOperationSPtr const &,
        ErrorCode const &)
    {
        // no-op unless overridden
    }

    void EntreeService::NamingRequestAsyncOperationBase::OnNameNotFound(AsyncOperationSPtr const & thisSPtr)
    {
        TryComplete(thisSPtr, ErrorCodeValue::NameNotFound);
    }

    void EntreeService::NamingRequestAsyncOperationBase::RaiseResolutionErrorAndComplete(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const & error)
    {
        this->OnResolveNamingServiceNonRetryableError(thisSPtr, error);
        this->TryComplete(thisSPtr, error);
    }
}
