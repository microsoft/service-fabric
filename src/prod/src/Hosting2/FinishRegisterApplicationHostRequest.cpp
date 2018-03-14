// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

FinishRegisterApplicationHostRequest::FinishRegisterApplicationHostRequest() 
    : id_()
{
}

FinishRegisterApplicationHostRequest::FinishRegisterApplicationHostRequest(
    wstring const & applicationHostId)
    : id_(applicationHostId)
{
}

void FinishRegisterApplicationHostRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("FinishRegisterApplicationHostRequest { ");
    w.Write("ApplicationHostId = {0}", Id);
    w.Write("}");
}
