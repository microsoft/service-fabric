// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

RegisterFabricActivatorClientReply::RegisterFabricActivatorClientReply() 
    : error_()
{
}

RegisterFabricActivatorClientReply::RegisterFabricActivatorClientReply(
    ErrorCode error)
    : error_(error)
{
}

void RegisterFabricActivatorClientReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("RegisterFabricActivatorClientReply { ");
    w.Write("Error = {0}", error_);
    w.Write("}");
}
