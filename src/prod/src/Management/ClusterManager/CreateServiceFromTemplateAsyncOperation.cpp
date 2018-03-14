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


AsyncOperationSPtr CreateServiceFromTemplateAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    CreateServiceFromTemplateMessageBody body;
    if (this->Request.GetBody(body))
    {
        return this->Replica.BeginAcceptCreateServiceFromTemplate(
            body.ApplicationName,
            body.ServiceName,
            body.ServiceDnsName,
            ServiceModelTypeName(body.ServiceType),
            body.PackageActivationMode,
            std::move(const_cast<vector<byte>&>(body.InitializationData)),
            move(clientRequest),
            timeout,
            callback,
            root);
    }
    else
    {
        return this->Replica.RejectInvalidMessage(
            move(clientRequest),
            callback,
            root);
    }
}

ErrorCode CreateServiceFromTemplateAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    return this->Replica.EndAcceptCreateServiceFromTemplate(operation);
}
