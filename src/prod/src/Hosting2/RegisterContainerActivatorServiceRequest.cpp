// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

RegisterContainerActivatorServiceRequest::RegisterContainerActivatorServiceRequest()
    : processId_()
    , listenAddress_()
{
}

RegisterContainerActivatorServiceRequest::RegisterContainerActivatorServiceRequest(
    DWORD processId,
    wstring const & listenAddress)
    : processId_(processId)
    , listenAddress_(listenAddress)
{
}

void RegisterContainerActivatorServiceRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("RegisterContainerActivatorServiceRequest { ");
    w.Write("ProcessId = {0}", processId_);
    w.Write("ListenAddress = {0}", listenAddress_);
    w.Write("}");
}
