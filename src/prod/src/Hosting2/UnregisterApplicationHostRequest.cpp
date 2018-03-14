// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

UnregisterApplicationHostRequest::UnregisterApplicationHostRequest()
    : id_()
{
}

UnregisterApplicationHostRequest::UnregisterApplicationHostRequest(wstring const & hostId)
    : id_(hostId)
{
}

void UnregisterApplicationHostRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("UnregisterApplicationHostRequest { ");
    w.Write("Id = {0}", Id);
    w.Write("}");
}
