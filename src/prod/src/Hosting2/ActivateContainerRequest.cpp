// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ActivateContainerRequest::ActivateContainerRequest()
    : activationArgs_()
    , timeoutTicks_()
{
}

ActivateContainerRequest::ActivateContainerRequest(
    ContainerActivationArgs && activationArgs,
    int64 timeoutTicks)
    : activationArgs_(move(activationArgs))
    , timeoutTicks_(timeoutTicks)
{
}

void ActivateContainerRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ActivateContainerRequest { ");
    w.Write("ActivationArgs = {0}", activationArgs_);
    w.Write("TimeoutTicks = {0}", timeoutTicks_);
    w.Write("}");
}
