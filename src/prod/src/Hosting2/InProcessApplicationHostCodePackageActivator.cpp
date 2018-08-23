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

StringLiteral const TraceType("InProcessApplicationHostCodePackageActivator");

InProcessApplicationHostCodePackageActivator::InProcessApplicationHostCodePackageActivator(
    ComponentRoot const & root,
    InProcessApplicationHost & appHost)
    : ApplicationHostCodePackageActivator(root)
    , appHost_(appHost)
{
}

AsyncOperationSPtr InProcessApplicationHostCodePackageActivator::BeginSendRequest(
    ApplicationHostCodePackageOperationRequest && requestBody,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(timeout);

    return this->AppHostMgr->BeginApplicationHostCodePackageOperation(
        requestBody,
        callback,
        parent);
}

ErrorCode InProcessApplicationHostCodePackageActivator::EndSendRequest(
    AsyncOperationSPtr const & operation,
    __out ApplicationHostCodePackageOperationReply & responseBody)
{
    auto error = this->AppHostMgr->EndApplicationHostCodePackageOperation(operation);

    responseBody = ApplicationHostCodePackageOperationReply(error);

    return ErrorCode::Success();
}

CodePackageContext InProcessApplicationHostCodePackageActivator::GetCodePackageContext()
{
    CodePackageContext codeContext;
    appHost_.GetCodePackageContext(codeContext);

    return codeContext;
}

ApplicationHostContext InProcessApplicationHostCodePackageActivator::GetHostContext()
{
    return appHost_.HostContext;
}

