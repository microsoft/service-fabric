// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

RegisterFabricRuntimeReply::RegisterFabricRuntimeReply()
    : errorCode_()
{
}

RegisterFabricRuntimeReply::RegisterFabricRuntimeReply(ErrorCode const & errorCode)
    : errorCode_(errorCode)
{
}

void RegisterFabricRuntimeReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("RegisterFabricRuntimeReply { ");
    w.Write("Error = {0}", Error);
    w.Write("}");
}
