// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace Reliability;
using namespace Naming;
using namespace std;

using namespace Management::ClusterManager;


AsyncOperationSPtr GetUpgradeStatusAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    UNREFERENCED_PARAMETER(clientRequest);
    UNREFERENCED_PARAMETER(timeout);

    ErrorCode error(ErrorCodeValue::Success);

    NamingUri body;
    if (this->Request.GetBody(body))
    {
        auto thisSPtr = shared_from_this();

        ApplicationUpgradeStatusDescription statusDescription;
        error = this->Replica.GetApplicationUpgradeStatus(
            body,
            dynamic_pointer_cast<ClientRequestAsyncOperation>(thisSPtr),
            statusDescription);

        if (error.IsSuccess())
        {
            this->SetReply(ClusterManagerMessage::GetClientOperationSuccess(statusDescription));
            this->TryComplete(thisSPtr, error);
        }
    }
    else
    {
        error = ErrorCodeValue::InvalidMessage;
    }

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        error,
        callback,
        root);
}

ErrorCode GetUpgradeStatusAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    return CompletedAsyncOperation::End(operation);

}

