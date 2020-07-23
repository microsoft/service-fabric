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

    StringLiteral const TraceComponent("ProcessRequest.CreateNetwork");

    const std::wstring EntreeService::CreateNetworkAsyncOperation::TraceId = L"CreateNetwork";

    EntreeService::CreateNetworkAsyncOperation::CreateNetworkAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
      : AdminRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::CreateNetworkAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        StartRequest(thisSPtr);
    }

    void EntreeService::CreateNetworkAsyncOperation::StartRequest(AsyncOperationSPtr const& thisSPtr)
    {
        if (!Management::NetworkInventoryManager::NetworkInventoryManagerConfig::IsNetworkInventoryManagerEnabled())
        {
            WriteError(
                TraceComponent,
                "{0}: CreateNetworkAsyncOperation, NetworkInventoryManager not configured",
                TraceId);

            this->TryComplete(thisSPtr, ErrorCodeValue::SystemServiceNotFound);
            return;
        }

        CreateNetworkMessageBody body;

        if (ReceivedMessage->GetBody(body))
        {
            ServiceModel::ModelV2::NetworkResourceDescription networkDescription = body.NetworkDescription;

            WriteInfo(
                TraceComponent,
                "{0}: CreateNetworkAsyncOperation, network name : {1}, network type : {2}",
                TraceId,
                networkDescription.Name,
                (int)networkDescription.NetworkType);

            auto operation = Properties.AdminClient.BeginCreateNetwork(
                    move(networkDescription),
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

    void EntreeService::CreateNetworkAsyncOperation::OnRequestCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        ErrorCode error = Properties.AdminClient.EndCreateNetwork(operation);
        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: CreateNetwork OnRequestCompleted, replying with error: {1}.",
                TraceId,
                error);

            this->SetReplyAndComplete(thisSPtr, NamingMessage::GetCreateNetworkReply(), error);
        }
        else if (IsRetryable(error))
        {
            WriteWarning(
                TraceComponent,
                "{0}: CreateNetwork OnRequestCompleted, replying with error: {1}.",
                TraceId,
                error);

            HandleRetryStart(thisSPtr);
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "{0}: CreateNetwork OnRequestCompleted, replying with error: {1}.",
                TraceId,
                error);

            TryComplete(thisSPtr, error);
        }
    }

    void EntreeService::CreateNetworkAsyncOperation::OnRetry(AsyncOperationSPtr const& thisSPtr)
    {
        StartRequest(thisSPtr);
    }
}
