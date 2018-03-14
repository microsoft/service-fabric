// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

DisableNodeRequest::DisableNodeRequest() 
    : disableReason_()
{
}

DisableNodeRequest::DisableNodeRequest(
    wstring const & reason)
    : disableReason_(reason)
{
}

void DisableNodeRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DisableNodeRequest { ");
    w.Write("DisableReason = {0}", disableReason_);
    w.Write("}");
}
