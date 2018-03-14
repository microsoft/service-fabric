// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

EnableNodeRequest::EnableNodeRequest() 
    : disableReason_()
{
}

EnableNodeRequest::EnableNodeRequest(
    wstring const & reason)
    : disableReason_(reason)
{
}

void EnableNodeRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("EnableNodeRequest { ");
    w.Write("DisableReason = {0}", disableReason_);
    w.Write("}");
}
