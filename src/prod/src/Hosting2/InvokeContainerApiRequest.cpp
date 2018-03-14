// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

InvokeContainerApiRequest::InvokeContainerApiRequest()
    : apiExecArgs_()
    , timeoutTicks_()
{
}

InvokeContainerApiRequest::InvokeContainerApiRequest(
    ContainerApiExecutionArgs && apiExecArgs,
    int64 timeoutTicks)
    : apiExecArgs_(move(apiExecArgs))
    , timeoutTicks_(timeoutTicks)
{
}

void InvokeContainerApiRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ActivateContainerRequest { ");
    w.Write("ApiExecArgs = {0}", apiExecArgs_);
    w.Write("TimeoutTicks = {0}", timeoutTicks_);
    w.Write("}");
}
