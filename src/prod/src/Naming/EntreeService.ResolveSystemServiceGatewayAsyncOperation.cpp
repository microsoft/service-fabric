// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Reliability;
    using namespace ServiceModel;
    using namespace SystemServices;
    using namespace Transport;

    StringLiteral const TraceComponent("ResolveSystemService");

    EntreeService::ResolveSystemServiceGatewayAsyncOperation::ResolveSystemServiceGatewayAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : RequestAsyncOperationBase(properties, move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::ResolveSystemServiceGatewayAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        ServiceResolutionMessageBody body;
        if (this->ReceivedMessage->GetBody(body))
        {
            auto serviceName = body.Name.ToString();
            if (SystemServiceApplicationNameHelper::IsPublicServiceName(serviceName))
            {
                TimedAsyncOperation::OnStart(thisSPtr);

                // Only need to use internal system service names for create/delete service.
                // The FM will convert from public names (fabric:/System/*) to internal
                // names when processing other requests.
                //
                targetServiceName_ = move(serviceName);

                this->Resolve(thisSPtr);
            }
            else
            {
                WriteWarning(
                    TraceComponent, 
                    "{0}: not a system service: name={1}",
                    this->TraceId,
                    serviceName);

                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidNameUri);
            }
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCode::FromNtStatus(this->ReceivedMessage->GetStatus()));
        }
    }

    void EntreeService::ResolveSystemServiceGatewayAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        this->Resolve(thisSPtr);
    }

    void EntreeService::ResolveSystemServiceGatewayAsyncOperation::Resolve(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->Properties.SystemServiceResolver->BeginResolve(
            targetServiceName_,
            this->ActivityHeader.ActivityId,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnResolveCompleted(operation, false); },
            thisSPtr);
        this->OnResolveCompleted(operation, true);
    }

    void EntreeService::ResolveSystemServiceGatewayAsyncOperation::OnResolveCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        vector<ServiceTableEntry> entries;
        PartitionInfo info;
        GenerationNumber generation;
        auto error = this->Properties.SystemServiceResolver->EndResolve(
            operation, 
            entries,
            info,
            generation);

        if (error.IsSuccess())
        {
            auto reply = NamingMessage::GetResolveSystemServiceReply(ResolvedServicePartition(
                false,
                entries[0],
                info,
                generation,
                -1,
                nullptr));
            this->SetReplyAndComplete(thisSPtr, move(reply), error);
        }
        else if (NamingErrorCategories::IsRetryableAtGateway(error))
        {
            WriteInfo(
                TraceComponent, 
                "{0}: retrying resolution of system service: name={1} error={2}",
                this->TraceId,
                targetServiceName_,
                error);

            this->HandleRetryStart(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }
}
