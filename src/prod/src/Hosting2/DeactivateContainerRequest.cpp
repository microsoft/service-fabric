// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

DeactivateContainerRequest::DeactivateContainerRequest()
    : deactivationArgs_()
    , timeoutTicks_()
{
}

DeactivateContainerRequest::DeactivateContainerRequest(
    ContainerDeactivationArgs && deactivationArgs,
    int64 timeoutTicks)
    : deactivationArgs_(move(deactivationArgs))
    , timeoutTicks_(timeoutTicks)
{
}

void DeactivateContainerRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DeactivateContainerRequest { ");
    w.Write("DeactivationArgs = {0}", deactivationArgs_);
    w.Write("TimeoutTicks = {0}", timeoutTicks_);
    w.Write("}");
}
