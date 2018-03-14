// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

RegisterFabricRuntimeRequest::RegisterFabricRuntimeRequest()
    : runtimeContext_()
{
}

RegisterFabricRuntimeRequest::RegisterFabricRuntimeRequest(
    FabricRuntimeContext const & runtimeContext)
    : runtimeContext_(runtimeContext)
{
}

void RegisterFabricRuntimeRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("RegisterFabricRuntimeRequest { ");
    w.Write("RuntimeContext = {0}", RuntimeContext);
    w.Write("}");
}
