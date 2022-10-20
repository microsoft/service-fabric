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
    using namespace Federation;
    using namespace Management::NetworkInventoryManager;

    StringLiteral const TraceComponent("ProcessRequest.DeleteNetwork");

    const std::wstring EntreeService::DeleteNetworkAsyncOperation::TraceId = L"DeleteNetwork";

    EntreeService::DeleteNetworkAsyncOperation::DeleteNetworkAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : AdminRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::DeleteNetworkAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        StartRequest(thisSPtr);
    }

    void EntreeService::DeleteNetworkAsyncOperation::StartRequest(AsyncOperationSPtr const& thisSPtr)
    {
        if (!Management::NetworkInventoryManager::NetworkInventoryManagerConfig::IsNetworkInventoryManagerEnabled())
        {
            WriteError(
                TraceComponent,
                "{0}: DeleteNetworkAsyncOperation, NetworkInventoryManager not configured",
                TraceId);

            this->TryComplete(thisSPtr, ErrorCodeValue::SystemServiceNotFound);
            return;
        }

        DeleteNetworkMessageBody body;
        if (ReceivedMessage->GetBody(body))
        {
            ServiceModel::DeleteNetworkDescription deleteNetworkDescription = body.DeleteNetworkDescription;

            WriteInfo(
                TraceComponent,
                "{0}: DeleteNetworkAsyncOperation, network name : {1}",
                TraceId,
                deleteNetworkDescription.NetworkName);

            auto operation = Properties.AdminClient.BeginDeleteNetwork(
                    move(deleteNetworkDescription),
                    ActivityHeader,
                    RemainingTime,
                    [this](AsyncOperationSPtr const& operation) { this->OnRequestCompleted(operation, false); },
                    thisSPtr);
                this->OnRequestCompleted(operation, true);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::DeleteNetworkAsyncOperation::OnRequestCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        ErrorCode error = Properties.AdminClient.EndDeleteNetwork(operation);
        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: DeleteNetwork OnRequestCompleted, replying with error: {1}.",
                TraceId,
                error);

            this->SetReplyAndComplete(thisSPtr, NamingMessage::GetDeleteNetworkReply(), error);
        }
        else if (IsRetryable(error))
        {
            WriteWarning(
                TraceComponent,
                "{0}: DeleteNetwork OnRequestCompleted, replying with error: {1}.",
                TraceId,
                error);

            HandleRetryStart(thisSPtr);
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "{0}: DeleteNetwork OnRequestCompleted, replying with error: {1}.",
                TraceId,
                error);

            TryComplete(thisSPtr, error);
        }
    }

    void EntreeService::DeleteNetworkAsyncOperation::OnRetry(AsyncOperationSPtr const& thisSPtr)
    {
        StartRequest(thisSPtr);
    }
}
