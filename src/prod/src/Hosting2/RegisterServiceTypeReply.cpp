// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

RegisterServiceTypeReply::RegisterServiceTypeReply()
    : errorCode_()
{
}

RegisterServiceTypeReply::RegisterServiceTypeReply(ErrorCode const & errorCode)
    : errorCode_(errorCode)
{
}

void RegisterServiceTypeReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("RegisterServiceTypeReply { ");
    w.Write("Error = {0}", Error);
    w.Write("}");
}
