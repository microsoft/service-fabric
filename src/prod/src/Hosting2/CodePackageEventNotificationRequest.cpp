// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

CodePackageEventNotificationRequest::CodePackageEventNotificationRequest()
    : codePackageEvent_()
{
}

CodePackageEventNotificationRequest::CodePackageEventNotificationRequest(
    CodePackageEventDescription const & eventDesc)
    : codePackageEvent_(eventDesc)
{
}

void CodePackageEventNotificationRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("CodePackageEventNotificationRequest { ");
    w.Write("CodePackageEvent = {0}, ", this->CodePackageEvent);
    w.Write("}");
}
