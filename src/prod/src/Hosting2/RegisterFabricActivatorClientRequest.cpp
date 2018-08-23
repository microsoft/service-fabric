// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

RegisterFabricActivatorClientRequest::RegisterFabricActivatorClientRequest() 
    : parentProcessId_(),
    nodeId_(),
    nodeInstanceId_()
{
}

RegisterFabricActivatorClientRequest::RegisterFabricActivatorClientRequest(
    DWORD parentProcessId,
    wstring const & nodeId,
    uint64 nodeInstanceId)
    : parentProcessId_(parentProcessId),
    nodeId_(nodeId),
    nodeInstanceId_(nodeInstanceId)
{
}

void RegisterFabricActivatorClientRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("RegisterFabricActivatorClientRequest { ");
    w.Write("ParentProcessId = {0}", parentProcessId_);
    w.Write("NodeId = {0}", nodeId_);
    w.Write("NodeInstanceId = {0}", nodeInstanceId_);
    w.Write("}");
}
