// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

RegisterServiceTypeRequest::RegisterServiceTypeRequest()
    : runtimeContext_(),
    serviceTypeInstanceId_()
{
}

RegisterServiceTypeRequest::RegisterServiceTypeRequest(
    FabricRuntimeContext const & runtimeContext,
    ServiceTypeInstanceIdentifier const & serviceTypeInstanceId)
    : runtimeContext_(runtimeContext),
    serviceTypeInstanceId_(serviceTypeInstanceId)
{
}

void RegisterServiceTypeRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("RegisterServiceTypeRequest { ");
    w.Write("RuntimeContext = {0}, ", RuntimeContext);
    w.Write("ServiceTypeInstanceId = {0}, ", ServiceTypeInstanceId);
    w.Write("}");
}
