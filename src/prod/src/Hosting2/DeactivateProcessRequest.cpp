// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

DeactivateProcessRequest::DeactivateProcessRequest() 
    : parentId_(),
    appServiceId_(),
    ticks_()
{
}

DeactivateProcessRequest::DeactivateProcessRequest(
    wstring const & parentId,
    wstring const & appServiceId,
    int64 timeoutTicks)
    : parentId_(parentId), 
    appServiceId_(appServiceId),
    ticks_(timeoutTicks)
{
}

void DeactivateProcessRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DeactivateProcessRequest { ");
    w.Write("ParentId = {0}", parentId_);
    w.Write("AppServiceId = {0}", appServiceId_);
    w.Write("TimeoutTicks = {0}", ticks_);
    w.Write("}");
}
