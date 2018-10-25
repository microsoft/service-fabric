// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

UnregisterApplicationHostRequest::UnregisterApplicationHostRequest()
    : activityDescription_()
    , id_()
{
}

UnregisterApplicationHostRequest::UnregisterApplicationHostRequest(
    Common::ActivityDescription const & activityDescription,
    wstring const & hostId)
    : activityDescription_(activityDescription)
    , id_(hostId)
{
}

void UnregisterApplicationHostRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("UnregisterApplicationHostRequest { ");
    w.Write("ActivityDescription = {0}", ActivityDescription);
    w.Write("Id = {0}", Id);
    w.Write("}");
}
