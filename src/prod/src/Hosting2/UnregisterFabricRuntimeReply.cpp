// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

UnregisterFabricRuntimeReply::UnregisterFabricRuntimeReply()
    : errorCode_()
{
}

UnregisterFabricRuntimeReply::UnregisterFabricRuntimeReply(ErrorCode const & errorCode)
    : errorCode_(errorCode)
{
}

void UnregisterFabricRuntimeReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("UnregisterFabricRuntimeReply { ");
    w.Write("Error = {0}", Error);
    w.Write("}");
}
