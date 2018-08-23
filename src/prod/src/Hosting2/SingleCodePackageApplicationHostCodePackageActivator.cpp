// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Transport;

StringLiteral const TraceType("SingleCodePackageApplicationHostCodePackageActivator");

SingleCodePackageApplicationHostCodePackageActivator::SingleCodePackageApplicationHostCodePackageActivator(
    ComponentRoot const & root,
    SingleCodePackageApplicationHost & appHost)
    : ApplicationHostCodePackageActivator(root)
    , appHost_(appHost)
{
}

AsyncOperationSPtr SingleCodePackageApplicationHostCodePackageActivator::BeginSendRequest(
    ApplicationHostCodePackageOperationRequest && requestBody,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto request = make_unique<Message>(requestBody);
    request->Headers.Add(Transport::ActorHeader(Actor::ApplicationHostManager));
    request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ApplicationHostCodePackageOperationRequest));

    return appHost_.Client.BeginRequest(
        move(request),
        timeout,
        callback,
        parent);
}

ErrorCode SingleCodePackageApplicationHostCodePackageActivator::EndSendRequest(
    AsyncOperationSPtr const & operation,
    __out ApplicationHostCodePackageOperationReply & responseBody)
{
    MessageUPtr reply;
    auto error = appHost_.Client.EndRequest(operation, reply);
    if (error.IsSuccess())
    {
        if (!reply->GetBody<ApplicationHostCodePackageOperationReply>(responseBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            
            WriteWarning(
                TraceType,
                Root.TraceId,
                "EndSendRequest: GetBody<ApplicationHostCodePackageOperationReply> failed: Message={0}",
                *reply);
        }
    }

    return error;
}

CodePackageContext SingleCodePackageApplicationHostCodePackageActivator::GetCodePackageContext()
{
    CodePackageContext codeContext;
    appHost_.GetCodePackageContext(codeContext);

    return codeContext;
}

ApplicationHostContext SingleCodePackageApplicationHostCodePackageActivator::GetHostContext()
{
    return appHost_.HostContext;
}



