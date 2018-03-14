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

    StringLiteral const TraceComponent("ResolveService");

    EntreeService::ResolveServiceAsyncOperation::ResolveServiceAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : ServiceResolutionAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::ResolveServiceAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        this->GetPsd(thisSPtr);
    }

    void EntreeService::ResolveServiceAsyncOperation::GetPsd(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = GetServiceDescriptionAsyncOperation::BeginGetCached(
            this->Properties,
            this->Name,
            this->ActivityHeader,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnGetPsdComplete(operation, false); },
            thisSPtr);
        this->OnGetPsdComplete(operation, true);
    }

    void EntreeService::ResolveServiceAsyncOperation::OnGetPsdComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        PartitionedServiceDescriptorSPtr psd;
        auto error = GetServiceDescriptionAsyncOperation::EndGetCached(operation, psd);

        if (error.IsSuccess())
        {
            this->ResolveLocations(thisSPtr, move(psd));
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }
}
