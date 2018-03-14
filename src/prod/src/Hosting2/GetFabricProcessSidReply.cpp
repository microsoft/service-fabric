// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

GetFabricProcessSidReply::GetFabricProcessSidReply() 
    : fabricProcessSid_(),
    fabricProcessId_(0)
{
}

GetFabricProcessSidReply::GetFabricProcessSidReply(
    wstring const & fabricProcessSid,
    DWORD fabricProcessId)
    : fabricProcessSid_(fabricProcessSid),
    fabricProcessId_(fabricProcessId)
{
}

void GetFabricProcessSidReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("GetFabricProcessSidReply { ");
    w.Write("Sid = {0}", FabricProcessSid);
    w.Write("ProcessId = {0}", FabricProcessId);
    w.Write("}");
}
