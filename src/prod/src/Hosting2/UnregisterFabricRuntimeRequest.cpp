// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

UnregisterFabricRuntimeRequest::UnregisterFabricRuntimeRequest()
    : id_()
{
}

UnregisterFabricRuntimeRequest::UnregisterFabricRuntimeRequest(wstring const & runtimeId)
    : id_(runtimeId)
{
}

void UnregisterFabricRuntimeRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("UnregisterFabricRuntimeRequest { ");
    w.Write("Id = {0}", Id);
    w.Write("}");
}
